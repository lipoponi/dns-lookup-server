#include <iostream>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <cstdio>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

int open_listen_fd(char* port) {
  addrinfo hints, *result_list, *ptr;
  int listen_fd, opt_val = 1;

  memset(&hints, 0, sizeof(addrinfo));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV;
  getaddrinfo(nullptr, port, &hints, &result_list);

  for (ptr = result_list; ptr != nullptr; ptr = ptr->ai_next) {
    if ((listen_fd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) < 0) {
      continue;
    }

    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (const void *) &opt_val, sizeof(int));

    if (bind(listen_fd, ptr->ai_addr, ptr->ai_addrlen) == 0) {
      break;
    }

    close(listen_fd);
  }

  freeaddrinfo(result_list);
  if (ptr == nullptr) {
    return -1;
  }

  if (listen(listen_fd, 1024) < 0) {
    perror("listen");
    close(listen_fd);
    return -1;
  }

  return listen_fd;
}

int echo(int connfd) {
  const int BLOCK_SIZE = 1024;
  char buf[BLOCK_SIZE];
  int n = recv(connfd, buf, BLOCK_SIZE, 0);

  if (n <= 0) {
    return n;
  }

  std::string hostname(buf, buf + n - 2);
  std::clog << hostname << std::endl;

  addrinfo hints, *result_list, *ptr;
  memset(&hints, 0, sizeof(addrinfo));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_UNSPEC;

  int rv = getaddrinfo(hostname.c_str(), "domain", &hints, &result_list);
  if (rv < 0) {
    std::cout << gai_strerror(rv) << std::endl;
    return 1;
  }

  for (ptr = result_list; ptr != nullptr; ptr = ptr->ai_next) {
    sockaddr_in *ip_addres = (struct sockaddr_in *)ptr->ai_addr;
    std::string ip(inet_ntoa(ip_addres->sin_addr));
    ip += "\r\n";

    send(connfd, ip.c_str(), ip.size(), 0);
  }

  return n;
}

int main() {
  int listenfd;
  socklen_t clientlen;
  struct sockaddr_storage clientaddr{};

  listenfd = open_listen_fd("15213");
  int epfd = epoll_create1(0);
  epoll_event listen_event{};
  listen_event.events = EPOLLIN;
  listen_event.data.fd = listenfd;
  epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &listen_event);

  const int MAX_EVENTS = 16;
  epoll_event events[MAX_EVENTS];

  while (true) {
    int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      perror("epoll_wait");
      exit(EXIT_FAILURE);
    }

    for (int i = 0; i < nfds; i++) {
      if (events[i].data.fd == listenfd) {
        clientlen = sizeof(struct sockaddr_storage);
        int connfd = accept(listenfd, (sockaddr *) &clientaddr, &clientlen);

        epoll_event event;
        memset(&event, 0, sizeof(epoll_event));
        event.events = EPOLLIN;
        event.data.fd = connfd;

        epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &event);
      } else {
        int result = echo(events[i].data.fd);
        if (result <= 0) {
          close(events[i].data.fd);
          epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
        }
      }
    }
  }

  return 0;
}