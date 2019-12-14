#ifndef ENDPOINT_H
#define ENDPOINT_H

#include <cassert>
#include <cstring>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <stdexcept>

typedef int unique_fd; // TODO: stop naeb :)

class endpoint {
  union {
    sockaddr_storage ss;
    sockaddr sa;
    sockaddr_in sin;
    sockaddr_in6 sin6;
  } storage;
  unique_fd socket_fd;

  endpoint(int address_family, const std::string &address, uint16_t port);

 public:
  static endpoint ipv4(const std::string &address, uint16_t port = 0);
  static endpoint ipv6(const std::string &address, uint16_t port = 0);

  endpoint() = delete;
  [[nodiscard]] sockaddr_storage get_sockaddr() const;
  unique_fd listen();
};

#endif //ENDPOINT_H
