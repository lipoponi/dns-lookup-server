#include "base_server.h"

base_server::base_server(const logger &log)
    : listen_fd(-1), epoll_fd(-1), log(log) {}

base_server::~base_server() {
  for (auto &conn : active_conn_threads) {
    eventfd_write(conn.first, RE_ABORT);
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
      shared_fd connection_fd = accept(listen_fd, (sockaddr *) &client, &len);
      address client_addr(client);
      id_t host_id = host_ids[client_addr.get_str()];

      if (MAX_CONN_PER_HOST <= host_active_conn_cnt[host_id]) {
        continue;
      }

      shared_fd event_fd = eventfd(0, 0);
      struct epoll_event finish_event = {.events = EPOLLIN | EPOLLET, .data = {.fd = event_fd}};
      epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event_fd, &finish_event);

      host_active_conn_cnt[host_id]++;
      conn_host[event_fd] = host_id;
      std::thread routine(&base_server::routine_wrapper, this, client_addr, connection_fd, event_fd);
      active_conn_threads[event_fd] = std::move(routine);
    } else {
      active_conn_threads[current].join();
      active_conn_threads.erase(current);
      host_active_conn_cnt[conn_host[current]]--;
      conn_host.erase(current);

      epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current, nullptr);
    }
  }
}

void base_server::loop() {
  for (;;) {
    exec();
  }
}

void base_server::routine_wrapper(const address &client, const shared_fd &connection_fd, const shared_fd &event_fd) {
  log.log("Connected {" + std::to_string(connection_fd) + "}: " + client.get_full_str());

  try {
    connection_routine(connection_fd, event_fd);
  } catch (std::runtime_error &e) {
    log.err("{" + std::to_string(connection_fd) + "}: " + e.what());
  }

  log.log("Disconnected {" + std::to_string(connection_fd) + "}: " + client.get_full_str());
}

void base_server::connection_routine(const shared_fd &connection_fd, const shared_fd &event_fd) {
  std::time_t start_time = std::time(nullptr);
  std::time_t now = start_time;

  shared_fd epfd = epoll_create1(0);
  struct epoll_event conn = {.events = EPOLLIN, .data = {.fd = connection_fd}};
  epoll_ctl(epfd, EPOLL_CTL_ADD, connection_fd, &conn);
  struct epoll_event cancel = {.events = EPOLLIN | EPOLLET, .data = {.fd = event_fd}};
  epoll_ctl(epfd, EPOLL_CTL_ADD, event_fd, &cancel);

  struct epoll_event events[MAX_EPOLL_EVENTS];
  bool abort = false;
  std::string buffer;

  while (!abort && (now = std::time(nullptr)) - start_time < MAX_TIME_PER_CONN) {
    std::time_t timeout = (MAX_TIME_PER_CONN - (now - start_time)) * 1000;

    int nfds = epoll_wait(epfd, events, MAX_EPOLL_EVENTS, timeout);
    if (nfds == -1) {
      throw std::runtime_error(strerror(errno));
    }

    for (int i = 0; i < nfds; i++) {
      int fd = events[i].data.fd;

      if (fd == connection_fd) {
        int rv = data_handler(connection_fd, buffer);
        abort |= (rv == 0);
      } else if (fd == event_fd) {
        eventfd_t val;
        eventfd_read(event_fd, &val);
        abort |= (val == RE_ABORT);
      }
    }
  }

  eventfd_write(event_fd, RE_DONE);
}
