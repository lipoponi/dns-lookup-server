#include "base_server.h"

BaseServer::BaseServer() : listen_fd(-1), epoll_fd(-1) {}

int BaseServer::setup(const std::string &hostname, const std::string &port) {
  struct addrinfo hints, *result_list, *ptr;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV;

  int rv = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result_list);
  if (rv < 0) {
    std::cerr << gai_strerror(rv) << std::endl;
    return EXIT_FAILURE;
  }

  for (ptr = result_list; ptr != nullptr; ptr = ptr->ai_next) {
    switch (ptr->ai_family) {
      case AF_INET: {
        char astring[INET_ADDRSTRLEN];
        auto *ip_address = (sockaddr_in *) ptr->ai_addr;
        auto *result = inet_ntop(AF_INET, &ip_address->sin_addr, astring, INET_ADDRSTRLEN);
        if (result != nullptr) {
          std::clog << "Trying " << astring << std::endl;
        }
        break;
      }
      case AF_INET6: {
        char astring[INET6_ADDRSTRLEN];
        auto *ip_addres = (sockaddr_in6 *) ptr->ai_addr;
        auto *result = inet_ntop(AF_INET6, &ip_addres->sin6_addr, astring, INET6_ADDRSTRLEN);
        if (result != nullptr) {
          std::clog << "Trying " << astring << std::endl;
        }
        break;
      }
    }

    int sock_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (sock_fd == -1) {
      perror("socket");
      continue;
    }

    int optval = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (const void *) &optval, sizeof(int));

    int bind_rv = bind(sock_fd, ptr->ai_addr, ptr->ai_addrlen);
    if (bind_rv == 0) {
      listen_fd = sock_fd;
      break;
    }

    perror("bind");
    close(sock_fd);
  }

  freeaddrinfo(result_list);
  if (listen_fd == -1) {
    std::cerr << "No suitable configurations" << std::endl;
    return EXIT_FAILURE;
  }

  if (listen(listen_fd, 1024) == -1) {
    perror("listen");
    close(listen_fd);
    listen_fd = -1;
    return EXIT_FAILURE;
  }

  epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    perror("epoll_create1");
    close(listen_fd);
    listen_fd = -1;
    return EXIT_FAILURE;
  }

  struct epoll_event listen_event;
  listen_event.events = EPOLLIN;
  listen_event.data.fd = listen_fd;
  rv = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &listen_event);

  if (rv == -1) {
    perror("epoll_ctl");
    close(epoll_fd);
    close(listen_fd);
    epoll_fd = listen_fd= -1;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int BaseServer::exec() {
  const int MAX_EVENTS = 1024;
  struct epoll_event events[MAX_EVENTS];

  int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
  if (nfds == -1) {
    perror("epoll_wait");
    return -1;
  }

  for (int i = 0; i < nfds; i++) {
    if (events[i].data.fd == listen_fd) {
      socklen_t clientlen = sizeof(struct sockaddr_storage);
      struct sockaddr_storage clientaddr;
      int connection_fd = accept(listen_fd, (sockaddr *) &clientaddr, &clientlen);

      auto *ip_address = (sockaddr_in *) &clientaddr;
      std::clog << "New [" << connection_fd << "]: " << inet_ntoa(ip_address->sin_addr) << std::endl;

      struct epoll_event event;
      memset(&event, 0, sizeof(struct epoll_event));
      event.events = EPOLLIN;
      event.data.fd = connection_fd;

      epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connection_fd, &event);
    } else {
      int rv = connection_handler(events[i].data.fd);
      if (rv <= 0) {
        close(events[i].data.fd);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
        std::clog << "Closed [" << events[i].data.fd << "]" << std::endl;
      }
    }
  }

  return 0;
}
