#ifndef GAI_SERVER_H
#define GAI_SERVER_H

#include <cstring>
#include <netdb.h>
#include <string>
#include <unordered_map>
#include <sstream>
#include <vector>

#include "base_server.h"

class gai_server : public base_server {
 public:
  gai_server();
  virtual ~gai_server() = default;
  int data_handler(const shared_fd &connection_fd, std::string &buffer) override;
  static int send_address_info(int connection_fd, const std::string &query);
  static std::vector<std::string> get_addresses(const std::string &query);
};

#endif //GAI_SERVER_H
