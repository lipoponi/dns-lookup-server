#include "base_server.h"

base_server::base_server(const logger &log)
    : listen_fd(-1), epoll_fd(-1), last_host_id(0), abort(false), log(log) {}

base_server::~base_server() {
  abort.store(true);

  for (auto &conn : active_conn_threads) {
    conn.second.join();
  }

  log.log("Server closed");
}

void base_server::setup(const std::string &addr, uint16_t port) {
  endpoint loopback = endpoint::ipv4(addr, port);
  listen_fd = loopback.listen();

  epoll_fd = epoll_create1(0);
  if (epoll_fd == -1) {
    throw std::runtime_error(strerror(errno));
  }

  struct epoll_event listen_event = {.events = EPOLLIN, .data = {.fd = listen_fd}};

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &listen_event) == -1) {
    throw std::runtime_error(strerror(errno));
  }

  log.log("Server running on " + loopback.get_address().get_full_str());
}

void base_server::exec() {
  struct epoll_event events[MAX_EPOLL_EVENTS];

  int nfds = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);
  if (nfds == -1) {
    throw std::runtime_error(strerror(errno));
  }

  for (int i = 0; i < nfds; i++) {
    int current = events[i].data.fd;

    if (current == listen_fd) {
      sockaddr_storage client{};
      socklen_t len = sizeof(client);
      int connection_fd = accept(listen_fd, (sockaddr *) &client, &len);
      if (connection_fd == -1) {
        throw std::runtime_error(strerror(errno));
      }
      address client_addr(client);
      std::string client_str = client_addr.get_str();

      if (client_host_ids.find(client_str) == client_host_ids.end()) {
        client_host_ids[client_str] = ++last_host_id;
      }
      id_t client_host_id = client_host_ids[client_str];

      if (MAX_CONN_PER_HOST <= host_active_conn_cnt[client_host_id]) {
        log.log("Ignored {" + std::to_string(connection_fd) + "}: " + client_addr.get_full_str());
        close(connection_fd);
        continue;
      }

      int event_fd = eventfd(0, 0);
      struct epoll_event finish_event = {.events = EPOLLIN | EPOLLET, .data = {.fd = event_fd}};
      epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event_fd, &finish_event);

      host_active_conn_cnt[client_host_id]++;
      conn_host[event_fd] = client_host_id;
      active_conn_threads.emplace(std::piecewise_construct,
                                  std::forward_as_tuple(event_fd),
                                  std::forward_as_tuple(&base_server::routine_wrapper,
                                                        this,
                                                        client_addr,
                                                        connection_fd,
                                                        event_fd));
    } else {
      active_conn_threads[current].join();
      active_conn_threads.erase(current);
      host_active_conn_cnt[conn_host[current]]--;
      conn_host.erase(current);

      epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current, nullptr);
      close(current);
    }
  }
}

void base_server::loop() {
  for (;;) {
    exec();
  }
}

void base_server::routine_wrapper(const address &client, const shared_fd &connection_fd, int event_fd) {
  log.log("Connected {" + std::to_string(connection_fd) + "}: " + client.get_full_str());

  try {
    connection_routine(connection_fd, event_fd);
  } catch (std::runtime_error &e) {
    log.err("{" + std::to_string(connection_fd) + "}: " + e.what());
  }

  log.log("Disconnected {" + std::to_string(connection_fd) + "}: " + client.get_full_str());
}

void base_server::connection_routine(const shared_fd &connection_fd, int event_fd) {
  std::time_t start_time = std::time(nullptr);
  std::time_t now;

  shared_fd epfd = epoll_create1(0);
  if (epfd == -1) {
    throw std::runtime_error(strerror(errno));
  }
  struct epoll_event conn = {.events = EPOLLIN, .data = {.fd = connection_fd}};
  epoll_ctl(epfd, EPOLL_CTL_ADD, connection_fd, &conn);

  struct epoll_event events[1];
  std::string buffer;

  now = std::time(nullptr);
  while (!abort.load() && now - start_time < MAX_TIME_PER_CONN) {
    std::time_t timeout = (MAX_TIME_PER_CONN - (now - start_time)) * 1000;

    int rv = epoll_wait(epfd, events, 1, timeout);
    if (rv == -1) {
      throw std::runtime_error(strerror(errno));
    } else if (rv == 0 || data_handler(connection_fd, buffer) == 0) {
      break;
    }

    now = std::time(nullptr);
  }

  eventfd_write(event_fd, RE_DONE);
}
