#include "gai_server.h"

gai_server::gai_server(const logger &log) : base_server(log) {}

int gai_server::data_handler(const shared_fd &connection_fd, std::string &buffer) {
  const int BLOCK_SIZE = 1024;
  char buf[BLOCK_SIZE];

  int n = recv(connection_fd, buf, BLOCK_SIZE, 0);

  if (n == -1) {
    throw std::runtime_error(strerror(errno));
  } else {
    buffer.reserve(buffer.size() + n);
    for (int i = 0; i < n; i++) {
      buffer.push_back(buf[i]);
      if (2 <= buffer.size() && *(buffer.end() - 2) == '\r' && *(buffer.end() - 1) == '\n') {
        std::string query(buffer.begin(), buffer.end() - 2);
        buffer.clear();

        log.log("{" + std::to_string(connection_fd) + "} ->: " + query);
        send_address_info(connection_fd, query);
      }
    }
  }

  return n;
}

int gai_server::send_address_info(int connection_fd, const std::string &query) {
  std::vector<std::string> result;

  try {
    result = get_addresses(query);
  } catch (std::runtime_error &e) {
    log.err("{" + std::to_string(connection_fd) + "}: " + query + " - " + e.what());
  }

  const std::string line_separator("\r\n");
  std::string response;

  for (auto &addr : result) {
    response.append(addr).append(line_separator);
  }

  response.append(line_separator);
  if (send(connection_fd, response.c_str(), response.size(), MSG_NOSIGNAL) == -1) {
    throw std::runtime_error(strerror(errno));
  }

  log.log("{" + std::to_string(connection_fd) + "} <-: Sent " + std::to_string(result.size()) + " lines");

  return 0;
}

std::vector<std::string> gai_server::get_addresses(const std::string &query) {
  std::vector<std::string> result;
  struct addrinfo hints{}, *result_list, *ptr;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_UNSPEC;

  int rv = getaddrinfo(query.c_str(), "domain", &hints, &result_list);
  if (rv != 0) {
    throw std::runtime_error(gai_strerror(rv));
  }

  for (ptr = result_list; ptr != nullptr; ptr = ptr->ai_next) {
    auto *storage = (sockaddr_storage *)ptr->ai_addr;
    address current(*storage);
    result.push_back(current.get_str());
  }

  freeaddrinfo(result_list);

  return result;
}
