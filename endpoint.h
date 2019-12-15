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

#include "address.h"
#include "shared_fd.h"

class endpoint {
  union {
    sockaddr_storage ss;
    sockaddr sa;
    sockaddr_in sin;
    sockaddr_in6 sin6;
  } storage;
  shared_fd socket_fd;

 public:
  static endpoint ipv4(const std::string &str, uint16_t port = 0);
  static endpoint ipv6(const std::string &str, uint16_t port = 0);

  endpoint() = delete;
  explicit endpoint(const address &addr);
  [[nodiscard]] sockaddr_storage get_sockaddr() const;
  [[nodiscard]] address get_address() const;
  shared_fd listen();
};

#endif //ENDPOINT_H