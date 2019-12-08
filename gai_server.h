#ifndef DNS_LOOKUP_SERVER__GAI_SERVER_H_
#define DNS_LOOKUP_SERVER__GAI_SERVER_H_

#include <cstring>
#include <netdb.h>
#include <string>
#include <unordered_map>

#include "base_server.h"

class GaiServer : public BaseServer {
 public:
  GaiServer();
  ~GaiServer() override = default;

  int connection_handler(int connection_fd) override;

 private:
  int send_address_info(int connection_fd, const std::string &query);

 private:
  std::unordered_map<int, std::string> buffer_table;
};

#endif //DNS_LOOKUP_SERVER__GAI_SERVER_H_
