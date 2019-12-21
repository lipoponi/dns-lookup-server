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
#include <ctime>
#include <algorithm>

#include "endpoint.h"
#include "smart_fd.h"
#include "logger.h"

#define MAX_EPOLL_EVENTS 1024

enum routine_event {
  RE_DONE = 1,
  RE_ABORT = 2
};

class base_server {
  static const size_t MAX_CONN_PER_HOST = 1;
  static const std::time_t MAX_TIME_PER_CONN = 5 * 60;

  shared_fd listen_fd;
  shared_fd epoll_fd;
  std::unordered_map<std::string, id_t> host_ids;
  std::unordered_map<int, std::thread> active_conn_threads;
  std::unordered_map<id_t, size_t> host_active_conn_cnt;
  std::unordered_map<int, id_t> conn_host;

 protected:
  logger log;

 public:
  explicit base_server(const logger &log = logger());
  virtual ~base_server();
  void setup(const std::string &addr = "127.0.0.1", uint16_t port = 0);
  void exec();
  void loop();
  void routine_wrapper(const address &client, const shared_fd &connection_fd, const shared_fd &event_fd);
  void connection_routine(const shared_fd &connection_fd, const shared_fd &event_fd);
  virtual int data_handler(const shared_fd &connection_fd, std::string &buffer) = 0;
};

#endif //BASE_SERVER_H
