#ifndef GAI_SERVER_H
#define GAI_SERVER_H

#include <cstring>
#include <netdb.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "base_server.h"
#include "logger.h"
#include "smart_fd.h"

class gai_server : public base_server {
  static const size_t MAX_RECV_SIZE = 4096;
  static const size_t MAX_BUFFER_SIZE = 4096;

 public:
  explicit gai_server(const logger &log = logger());
  ~gai_server() override = default;
  int data_handler(const shared_fd &connection_fd, std::string &buffer) override;
  void send_address_info(const shared_fd &connection_fd, const std::string &query);
  static std::vector<std::string> get_addresses(const std::string &query);
};

#endif //GAI_SERVER_H
