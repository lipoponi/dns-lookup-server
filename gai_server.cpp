#include "gai_server.h"

gai_server::gai_server() : base_server() {}

int gai_server::connection_handler(int connection_fd) {
  const int BLOCK_SIZE = 1024;
  char buf[BLOCK_SIZE];

  int n = recv(connection_fd, buf, BLOCK_SIZE, 0);

  if (n < 0) {
    perror("recv");
    return -1;
  } else if (n == 0) {
    buffer_table.erase(connection_fd);
    return -1;
  } else {
    std::string &connection_buffer = buffer_table[connection_fd];

    for (int i = 0; i < n; i++) {
      connection_buffer.push_back(buf[i]);
      if (2 <= connection_buffer.size()
          && *(connection_buffer.end() - 2) == '\r'
          && *(connection_buffer.end() - 1) == '\n') {
        std::string query(connection_buffer.begin(), connection_buffer.end() - 2);
        connection_buffer.clear();

        std::thread th(&gai_server::send_address_info, this, connection_fd, query);
        std::thread gh(&gai_server::send_address_info, this, connection_fd, query);
        th.detach();
        gh.detach();
      }
    }
  }

  return n;
}

int gai_server::send_address_info(int connection_fd, const std::string &query) {
  struct addrinfo hints, *result_list, *ptr;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_UNSPEC;

  int rv = getaddrinfo(query.c_str(), "domain", &hints, &result_list);
  if (rv != 0) {
    std::cerr << "getaddrinfo: " << gai_strerror(rv) << std::endl;
    result_list = nullptr;
  }

  char line_separator[2] = {'\r', '\n'};
  std::string response;

  for (ptr = result_list; ptr != nullptr; ptr = ptr->ai_next) {
    char *astring = nullptr;

    switch (ptr->ai_family) {
      case AF_INET: {
        astring = new char[INET_ADDRSTRLEN];
        auto *ip_address = (sockaddr_in *) ptr->ai_addr;
        auto *result = inet_ntop(AF_INET, &ip_address->sin_addr, astring, INET_ADDRSTRLEN);
        if (result == nullptr) {
          delete[] astring;
          astring = nullptr;
        }
        break;
      }
      case AF_INET6: {
        astring = new char[INET6_ADDRSTRLEN];
        auto *ip_addres = (sockaddr_in6 *) ptr->ai_addr;
        auto *result = inet_ntop(AF_INET6, &ip_addres->sin6_addr, astring, INET6_ADDRSTRLEN);
        if (result == nullptr) {
          delete[] astring;
          astring = nullptr;
        }
        break;
      }
    }

    response.append(astring);
    delete[] astring;
    response.append("\r\n");
  }

  response.append("\r\n");
  send(connection_fd, response.c_str(), response.size(), 0);
  return 0;
}