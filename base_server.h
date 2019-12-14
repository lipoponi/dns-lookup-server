#ifndef DNS_LOOKUP_SERVER__BASE_SERVER_H_
#define DNS_LOOKUP_SERVER__BASE_SERVER_H_

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "endpoint.h"

class BaseServer {
 public:
  BaseServer();
  virtual ~BaseServer() = default;

  int setup();
  int exec();
  virtual int connection_handler(int connection_fd) = 0;

 private:
  shared_fd listen_fd;
  int epoll_fd;
};

#endif //DNS_LOOKUP_SERVER__BASE_SERVER_H_
