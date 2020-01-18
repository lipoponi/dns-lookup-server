#ifndef APP_H
#define APP_H

#include <atomic>
#include <list>
#include <mutex>
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <thread>
#include <unordered_map>
#include <vector>

#include "address.h"
#include "endpoint.h"
#include "logger.h"
#include "unique_fd.h"

typedef std::unordered_map<std::string, std::vector<id_t>> query_collector_t;

class app {
  struct connection_t {
    unique_fd fd;
    address client;
    std::string buffer;
  };
  struct query_t {
    query_t() : done(false) {}

    std::thread th;
    std::atomic<bool> done;
  };

  static const size_t MAX_EPOLL_EVENTS = 1024;
  static const size_t MAX_RECV_SIZE = 4096;
  static const size_t MAX_QUERY_LEN = 1024;

  logger log;
  unique_fd listen_fd;
  unique_fd epoll_fd;
  std::mutex conn_m;
  std::unordered_map<id_t, connection_t> connections;
  id_t last_connection_id;
  std::list<query_t> queries;

  void clean_query_threads();
  id_t add_connection(const address &client, unique_fd &&connection_fd);
  void send_by_id(id_t connection_id, const std::string &content);
  void remove_connection(id_t connection_id);

  void step();
  bool data_handler(id_t connection_id, query_collector_t &query_collector);
  void query_handler(const std::string &query, const std::vector<id_t> &receivers, std::atomic<bool> *done);
  static std::vector<std::string> get_addresses(const std::string &domain);

 public:
  explicit app(const logger &log);
  ~app();
  void setup(const std::string &addr = "127.0.0.1", uint16_t port = 0);
  void run();
};

#endif //APP_H
