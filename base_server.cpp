#include "base_server.h"

BaseServer::BaseServer() : listen_fd(-1), epoll_fd(-1) {}

int BaseServer::setup() {
  endpoint loopback = endpoint::ipv4("127.0.0.1", 15213);
  listen_fd = loopback.listen();

  epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    perror("epoll_create");
    close(listen_fd);
    listen_fd = -1;
    return -1;
  }

  struct epoll_event listen_event = {.events = EPOLLIN, .data = {.fd = listen_fd}};

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &listen_event) == -1) {
    perror("epoll_ctl");
    close(epoll_fd);
    close(listen_fd);
    epoll_fd = listen_fd = -1;
    return -1;
  }

  return 0;
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
