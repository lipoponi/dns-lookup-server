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
#include <unordered_map>
#include <mutex>
#include <csignal>

std::unordered_map<int, std::string> buffers;
volatile bool cancel = false;

int open_listen_fd(char *port) {
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

int gai(const std::string &hostname, int connfd) {
  addrinfo hints, *result_list, *ptr;
  memset(&hints, 0, sizeof(addrinfo));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_UNSPEC;

  int rv = getaddrinfo(hostname.c_str(), "domain", &hints, &result_list);
  if (rv < 0) {
    std::cout << gai_strerror(rv) << std::endl;
    return 1;
  }

  char line_separator[2] = {'\r', '\n'};

  for (ptr = result_list; ptr != nullptr; ptr = ptr->ai_next) {
    char *astring = nullptr;

    switch (ptr->ai_family) {
      case AF_INET: {
        astring = new char[INET_ADDRSTRLEN];
        auto *ip_address = (sockaddr_in *) ptr->ai_addr;
        auto *result = inet_ntop(AF_INET, &ip_address->sin_addr, astring, INET_ADDRSTRLEN);
        if (result == nullptr) {
          delete[] astring;
          astring = nullptr;
        }
        break;
      }
      case AF_INET6: {
        astring = new char[INET6_ADDRSTRLEN];
        auto *ip_addres = (sockaddr_in6 *) ptr->ai_addr;
        auto *result = inet_ntop(AF_INET6, &ip_addres->sin6_addr, astring, INET6_ADDRSTRLEN);
        if (result == nullptr) {
          delete[] astring;
          astring = nullptr;
        }
        break;
      }
    }

    std::string ip(astring);
    delete[] astring;
    send(connfd, ip.c_str(), ip.size(), 0);
    send(connfd, line_separator, sizeof(line_separator), 0);
  }

  send(connfd, line_separator, sizeof(line_separator), 0);
}

int echo(int connfd) {
  const int BLOCK_SIZE = 1024;
  char buf[BLOCK_SIZE];
  int n = recv(connfd, buf, BLOCK_SIZE, 0);

  if (0 < n) {
    buffers[connfd].reserve(buffers[connfd].size() + n);
    for (int i = 0; i < n; i++) {
      buffers[connfd].push_back(buf[i]);

      if (2 <= buffers[connfd].size() && *(buffers[connfd].end() - 2) == '\r' && *(buffers[connfd].end() - 1) == '\n') {
        gai(std::string(buffers[connfd].begin(), buffers[connfd].end() - 2), connfd);
        std::cout << connfd << " says: " << std::string(buffers[connfd].begin(), buffers[connfd].end() - 2)
                  << std::endl;
        buffers[connfd].clear();
      }
    }
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

        auto *ip_addres = (sockaddr_in *) &clientaddr;
        std::clog << "New connection " << connfd << " ip: " << inet_ntoa(ip_addres->sin_addr) << std::endl;

        epoll_event event;
        memset(&event, 0, sizeof(epoll_event));
        event.events = EPOLLIN;
        event.data.fd = connfd;

        epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &event);
      } else {
        int result = echo(events[i].data.fd);
        if (result <= 0) {
          buffers[events[i].data.fd].clear();
          close(events[i].data.fd);
          epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
          std::clog << "Disconnected " << events[i].data.fd << std::endl;
        }
      }
    }
  }

  return 0;
}