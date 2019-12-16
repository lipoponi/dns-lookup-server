#ifndef BASE_SERVER_H
#define BASE_SERVER_H

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <string>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

#include "endpoint.h"

class base_server {
  shared_fd listen_fd;
  shared_fd epoll_fd;
  std::unordered_map<int, std::thread> connections;
  std::unordered_map<int, shared_fd> conn_fds;

 public:
  base_server();
  virtual ~base_server() {
    for (auto &con : connections) {
      eventfd_write(con.first, 2);
      con.second.join();
    }
  }
  void setup(const std::string &addr = "127.0.0.1", uint16_t port = 0);
  void exec();
  void loop();
  void routine_wrapper(const address &client, const shared_fd &connection_fd, const shared_fd &event_fd);
  void connection_routine(const shared_fd &connection_fd, const shared_fd &event_fd);
  virtual int data_handler(const shared_fd &connection_fd, std::string &buffer) = 0;
};

#endif //BASE_SERVER_H
