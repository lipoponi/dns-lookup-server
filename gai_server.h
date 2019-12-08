#ifndef DNS_LOOKUP_SERVER__GAI_SERVER_H_
#define DNS_LOOKUP_SERVER__GAI_SERVER_H_

#include "base_server.h"

class GaiServer : public BaseServer {
 public:
  GaiServer();
  ~GaiServer() override = default;

  int connection_handler(int connection_fd) override;
};

#endif //DNS_LOOKUP_SERVER__GAI_SERVER_H_
