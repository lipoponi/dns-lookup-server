#include "gai_server.h"

GaiServer::GaiServer() : BaseServer() {}

int GaiServer::connection_handler(int connection_fd) {
  const int BLOCK_SIZE = 1024;
  char buf[BLOCK_SIZE];

  int n = recv(connection_fd, buf, BLOCK_SIZE, 0);

  if (0 < n) {
    std::clog << "[" << connection_fd << "] received: \"" << std::string(buf, buf + n) << "\"" << std::endl;
    send(connection_fd, buf, n, 0);
    std::clog << "[" << connection_fd << "] sent: \"" << std::string(buf, buf + n) << "\"" << std::endl;
  }

  return n;
}