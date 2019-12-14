#ifndef BASE_SERVER_H
#define BASE_SERVER_H

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <thread>
#include <unordered_map>

#include "endpoint.h"

class base_server {
  const int timeout = 1000;
  shared_fd listen_fd;
  shared_fd epoll_fd;
  std::unordered_map<std::string, std::thread> threads;

 public:
  base_server();
  virtual ~base_server() = default;
  void setup();
  void exec();
  void loop();
  virtual int connection_handler(int connection_fd) = 0;
};

#endif //BASE_SERVER_H
