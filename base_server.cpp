#include "base_server.h"

base_server::base_server() : listen_fd(-1), epoll_fd(-1) {}

void base_server::setup() {
  endpoint loopback = endpoint::ipv4("127.0.0.1");
  listen_fd = loopback.listen();

  std::cout << "Server running on " << loopback.get_address().get_full_str() << std::endl;

  epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    throw std::runtime_error(strerror(errno));
  }

  struct epoll_event listen_event = {.events = EPOLLIN, .data = {.fd = listen_fd}};

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &listen_event) == -1) {
    throw std::runtime_error(strerror(errno));
  }
}

void base_server::exec() {
  const int MAX_EVENTS = 1024;
  struct epoll_event events[MAX_EVENTS];

  int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
  if (nfds == -1) {
    throw std::runtime_error(strerror(errno));
  }

  for (int i = 0; i < nfds; i++) {
    if (events[i].data.fd == listen_fd) {
      union {
        sockaddr_storage ss;
        sockaddr_in sin;
        sockaddr_in6 sin6;
      } client{};
      socklen_t len = sizeof(client);
      int connection_fd = accept(listen_fd, (sockaddr *) &client, &len);

      std::string ip;
      uint16_t port = -1;
      char buf[40];

      switch (client.ss.ss_family) {
        case AF_INET:
          ip = inet_ntop(AF_INET, &client.sin.sin_addr, buf, sizeof(buf));
          port = ntohs(client.sin.sin_port);
          break;
        case AF_INET6:
          ip = inet_ntop(AF_INET6, &client.sin6.sin6_addr, buf, sizeof(buf));
          port = ntohs(client.sin6.sin6_port);
          break;
      }

      std::cout << "New [" << connection_fd << "]: " << ip << ":" << port << std::endl;

      struct epoll_event event = {.events = EPOLLIN, .data = {.fd = connection_fd}};

      epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connection_fd, &event);
    } else {
      int rv = connection_handler(events[i].data.fd);
      if (rv <= 0) {
        close(events[i].data.fd);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
        std::cout << "Closed [" << events[i].data.fd << "]" << std::endl;
      }
    }
  }
}

void base_server::loop() {
  for (;;) {
    exec();
  }
}
