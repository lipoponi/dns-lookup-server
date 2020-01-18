#include "address.h"

address::address()
    : storage() {
  memset(&storage, 0, sizeof(storage));
}

address::address(const int address_family, const std::string &str, uint16_t port)
    : storage() {
  assert(address_family == AF_INET || address_family == AF_INET6);
  memset(&storage, 0, sizeof(storage));

  storage.ss.ss_family = address_family;
  void *buf = address_family == AF_INET ? (void *) &storage.sin.sin_addr : (void *) &storage.sin6.sin6_addr;

  switch (inet_pton(address_family, str.c_str(), buf)) {
    case 0: { throw std::runtime_error("Invalid address"); }
    case -1: { throw std::runtime_error(strerror(errno)); }
  }

  if (address_family == AF_INET) {
    storage.sin.sin_port = htons(port);
  } else {
    storage.sin6.sin6_port = htons(port);
  }
}

address address::ipv4(const std::string &str, uint16_t port) {
  return address(AF_INET, str, port);
}

address address::ipv6(const std::string &str, uint16_t port) {
  return address(AF_INET6, str, port);
}

address::address(sockaddr_storage ss)
    : storage{.ss = ss} {}

sockaddr_storage address::get_sockaddr() const {
  return storage.ss;
}

std::string address::get_str() const {
  char buf[INET6_ADDRSTRLEN];
  int af = storage.ss.ss_family;
  void *ptr = (af == AF_INET ? (void *) &storage.sin.sin_addr : (void *) &storage.sin6.sin6_addr);

  if (inet_ntop(af, ptr, buf, sizeof(buf)) == nullptr) {
    throw std::runtime_error(strerror(errno));
  }

  return buf;
}

uint16_t address::get_port() const {
  switch (storage.ss.ss_family) {
    case AF_INET: { return ntohs(storage.sin.sin_port); }
    case AF_INET6: { return ntohs(storage.sin6.sin6_port); }
    default: { throw std::runtime_error("Invalid address family"); }
  }
}

std::string address::get_full_str() const {
  switch (storage.ss.ss_family) {
    case AF_INET: { return get_str() + ':' + std::to_string(get_port()); }
    case AF_INET6: { return "[" + get_str() + "]:" + std::to_string(get_port()); }
    default: { throw std::runtime_error("Invalid address family"); }
  }
}
