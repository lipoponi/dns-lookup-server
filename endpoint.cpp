#include "endpoint.h"

endpoint::endpoint(const int address_family, const std::string &address, const uint16_t port)
    : storage(), socket_fd(-1) {
  assert(address_family == AF_INET || address_family == AF_INET6);
  memset(&storage, 0, sizeof(storage));

  storage.ss.ss_family = address_family;
  void *buf = address_family == AF_INET ? (void *) &storage.sin.sin_addr : (void *) &storage.sin6.sin6_addr;

  if (inet_pton(address_family, address.c_str(), buf) == -1) {
    throw std::runtime_error(strerror(errno));
  }

  if (address_family == AF_INET) {
    storage.sin.sin_port = htons(port);
  } else {
    storage.sin6.sin6_port = htons(port);
  }
}

endpoint endpoint::ipv4(const std::string &address, const uint16_t port) {
  return endpoint(AF_INET, address, port);
}

endpoint endpoint::ipv6(const std::string &address, const uint16_t port) {
  return endpoint(AF_INET6, address, port);
}

sockaddr_storage endpoint::get_sockaddr() const {
  return storage.ss;
}

unique_fd endpoint::listen() {
  const int af = storage.ss.ss_family;

  socket_fd = socket(af, SOCK_STREAM, IPPROTO_TCP);
  if (socket_fd == -1) {
    throw std::runtime_error(strerror(errno));
  }

  int reuseaddr = 1;
  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1) {
    throw std::runtime_error(strerror(errno));
  }

  const socklen_t len = af == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
  if (bind(socket_fd, &storage.sa, len) == -1) {
    throw std::runtime_error(strerror(errno));
  }

  if (::listen(socket_fd, SOMAXCONN) == -1) {
    throw std::runtime_error(strerror(errno));
  }

  return socket_fd;
}