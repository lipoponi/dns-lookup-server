#ifndef GAI_SERVER_H
#define GAI_SERVER_H

#include <cstring>
#include <netdb.h>
#include <string>
#include <unordered_map>
#include <sstream>

#include "base_server.h"

class gai_server : public base_server {
 public:
  gai_server();
  ~gai_server() override = default;

  int connection_handler(int connection_fd) override;

 private:
  int send_address_info(int connection_fd, const std::string &query);

 private:
  std::unordered_map<int, std::string> buffer_table;
};

#endif //GAI_SERVER_H
