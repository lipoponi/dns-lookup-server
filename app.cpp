#include "app.h"

app::app(const logger &log)
    : log(log), last_connection_id(0) {}

app::~app() {
  connections.clear();
  for (auto &q : queries) {
    q.th.join();
  }
}

void app::setup(const std::string &addr, uint16_t port) {
  endpoint epoint = endpoint::ipv4(addr, port);
  listen_fd = epoint.listen();

  epoll_fd = unique_fd(epoll_create1(0));
  if (epoll_fd.fd() == -1) {
    throw std::runtime_error(strerror(errno));
  }

  struct epoll_event listen_event = {.events = EPOLLIN, .data = {.u64 = 0}};
  if (epoll_ctl(epoll_fd.fd(), EPOLL_CTL_ADD, listen_fd.fd(), &listen_event) == -1) {
    throw std::runtime_error(strerror(errno));
  }

  log.log("Server running on " + epoint.get_address().get_full_str());
}

void app::run() {
  for (;;) {
    step();
  }
}

void app::clean_query_threads() {
  auto it = queries.begin();
  while (it != queries.end()) {
    auto tmp = it;
    it++;

    if (tmp->done.load()) {
      tmp->th.join();
      queries.erase(tmp);
    }
  }
}

id_t app::add_connection(const address &client, unique_fd &&connection_fd) {
  std::lock_guard<std::mutex> lg(conn_m);

  id_t new_connection_id = ++last_connection_id;

  struct epoll_event event = {.events = EPOLLIN, .data = {.u64 = new_connection_id}};
  if (epoll_ctl(epoll_fd.fd(), EPOLL_CTL_ADD, connection_fd.fd(), &event) == -1) {
    throw std::runtime_error(strerror(errno));
  }

  connection_t g{.fd = std::move(connection_fd), .client = client};
  connections.emplace(new_connection_id, std::move(g));

  log.log("Connected {" + std::to_string(new_connection_id) + "}: " + client.get_full_str());

  return last_connection_id;
}

void app::send_by_id(id_t connection_id, const std::string &content) {
  std::lock_guard<std::mutex> lg(conn_m);

  if (connections.find(connection_id) == connections.end()) {
    return;
  }

  if (send(connections[connection_id].fd.fd(), content.c_str(), content.size(), MSG_NOSIGNAL) == -1) {
    log.err("Failed send to {" + std::to_string(connection_id) + "}: " + strerror(errno));
  } else {
    log.log("Sent " + std::to_string(content.size()) + " bytes to {" + std::to_string(connection_id) + "}");
  }
}

void app::remove_connection(id_t connection_id) {
  std::lock_guard<std::mutex> lg(conn_m);

  std::string client_address = connections[connection_id].client.get_full_str();

  if (epoll_ctl(epoll_fd.fd(), EPOLL_CTL_DEL, connections[connection_id].fd.fd(), nullptr) == -1) {
    throw std::runtime_error(strerror(errno));
  }
  connections.erase(connection_id);

  log.log("Disconnected {" + std::to_string(connection_id) + "}: " + client_address);
}

void app::step() {
  struct epoll_event events[MAX_EPOLL_EVENTS];

  int n_events = epoll_wait(epoll_fd.fd(), events, MAX_EPOLL_EVENTS, -1);
  if (n_events == -1) {
    throw std::runtime_error(strerror(errno));
  }

  query_collector_t query_collector;

  for (int i = 0; i < n_events; i++) {
    auto current = events[i].data.u64;

    try {
      if (current == 0) {
        struct sockaddr_storage client{};
        socklen_t len = sizeof(client);
        unique_fd connection_fd(accept(listen_fd.fd(), (sockaddr *) &client, &len));
        if (connection_fd.fd() == -1) {
          throw std::runtime_error(strerror(errno));
        }

        address client_info(client);
        add_connection(client_info, std::move(connection_fd));
      } else {
        bool rv = data_handler(current, query_collector);
        if (!rv) {
          remove_connection(current);
        }
      }
    } catch (std::exception &e) {
      log.err("{" + std::to_string(current) + "}: " + e.what());
    }
  }

  clean_query_threads();

  for (auto &pr : query_collector) {
    queries.emplace_back();
    queries.back().th = std::thread(&app::query_handler, this, pr.first, pr.second, &queries.back().done);
  }
}

bool app::data_handler(id_t connection_id, query_collector_t &query_collector) {
  char buf[MAX_RECV_SIZE];

  connection_t &connection = connections[connection_id];
  int n = recv(connection.fd.fd(), buf, sizeof(buf), 0);
  if (n == -1) {
    throw std::runtime_error(strerror(errno));
  }

  for (int i = 0; i < n; i++) {
    std::string &buffer = connection.buffer;

    buffer.push_back(buf[i]);
    if (MAX_QUERY_LEN <= buffer.size()) {
      throw std::runtime_error("Query string is too long");
    }

    if (2 <= buffer.size() && *(buffer.end() - 2) == '\r' && *(buffer.end() - 1) == '\n') {
      std::string query(buffer.begin(), buffer.end() - 2);
      buffer.clear();

      log.log("{" + std::to_string(connection_id) + "}->: " + query);
      query_collector[query].push_back(connection_id);
    }
  }

  return n;
}

void app::query_handler(const std::string &query, const std::vector<id_t> &receivers, std::atomic<bool> *done) {
  std::string response;

  try {
    std::vector<std::string> response_list = get_addresses(query);
    for (const std::string &address : response_list) {
      response.append(address);
      response.append("\r\n");
    }
  } catch (std::exception &e) {
    log.err("'" + query + "': " + e.what());
  }

  response.append("\r\n");
  for (auto id : receivers) {
    send_by_id(id, response);
  }

  done->store(true);
}

std::vector<std::string> app::get_addresses(const std::string &domain) {
  std::vector<std::string> result;
  struct addrinfo hints{}, *result_list, *ptr;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_UNSPEC;

  int rv = getaddrinfo(domain.c_str(), "domain", &hints, &result_list);
  if (rv != 0) {
    throw std::runtime_error(gai_strerror(rv));
  }

  for (ptr = result_list; ptr != nullptr; ptr = ptr->ai_next) {
    auto *storage = (sockaddr_storage *) ptr->ai_addr;
    address current(*storage);
    result.push_back(current.get_str());
  }

  freeaddrinfo(result_list);

  return result;
}