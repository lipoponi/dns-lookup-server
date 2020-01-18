#include "endpoint.h"

endpoint endpoint::ipv4(const std::string &str, const uint16_t port) {
  return endpoint(address::ipv4(str, port));
}

endpoint endpoint::ipv6(const std::string &str, const uint16_t port) {
  return endpoint(address::ipv6(str, port));
}

endpoint::endpoint(const address &addr)
    : storage{.ss = addr.get_sockaddr()} {}

sockaddr_storage endpoint::get_sockaddr() const {
  return storage.ss;
}

address endpoint::get_address() const {
  return address(storage.ss);
}
unique_fd endpoint::listen() {
  const int af = storage.ss.ss_family;

  socket_fd = unique_fd(socket(af, SOCK_STREAM, IPPROTO_TCP));
  if (socket_fd.fd() == -1) {
    throw std::runtime_error(strerror(errno));
  }

  int reuseaddr = 1;
  if (setsockopt(socket_fd.fd(), SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1) {
    throw std::runtime_error(strerror(errno));
  }

  socklen_t len = af == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
  if (bind(socket_fd.fd(), &storage.sa, len) == -1) {
    throw std::runtime_error(strerror(errno));
  }

  if (::listen(socket_fd.fd(), SOMAXCONN) == -1) {
    throw std::runtime_error(strerror(errno));
  }

  if (getsockname(socket_fd.fd(), &storage.sa, &len) == -1) {
    throw std::runtime_error(strerror(errno));
  }

  return std::move(socket_fd);
}
