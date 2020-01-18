#ifndef ADDRESS_H
#define ADDRESS_H

#include <arpa/inet.h>
#include <cassert>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>

class address {
  union {
    sockaddr_storage ss;
    sockaddr sa;
    sockaddr_in sin;
    sockaddr_in6 sin6;
  } storage;

  address(int address_family, const std::string &str, uint16_t port);

 public:
  static address ipv4(const std::string &str, uint16_t port);
  static address ipv6(const std::string &str, uint16_t port);

  address();
  explicit address(sockaddr_storage ss);
  [[nodiscard]] sockaddr_storage get_sockaddr() const;
  [[nodiscard]] std::string get_str() const;
  [[nodiscard]] uint16_t get_port() const;
  [[nodiscard]] std::string get_full_str() const;
};

#endif //ADDRESS_H_
