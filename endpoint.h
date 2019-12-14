#ifndef ENDPOINT_H
#define ENDPOINT_H

#include <arpa/inet.h>
#include <cassert>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

#include "shared_fd.h"

class endpoint {
  union {
    sockaddr_storage ss;
    sockaddr sa;
    sockaddr_in sin;
    sockaddr_in6 sin6;
  } storage;
  shared_fd socket_fd;

  endpoint(int address_family, const std::string &address, uint16_t port);

 public:
  static endpoint ipv4(const std::string &address, uint16_t port = 0);
  static endpoint ipv6(const std::string &address, uint16_t port = 0);

  endpoint() = delete;
  [[nodiscard]] sockaddr_storage get_sockaddr() const;
  shared_fd listen();
};

#endif //ENDPOINT_H
