#include "base_server.h"

base_server::base_server(const logger &log)
    : listen_fd(-1), epoll_fd(-1), log(log) {}

base_server::~base_server() {
  for (auto &con : connections) {
    eventfd_write(con.first, 2);
    con.second.join();
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
  struct epoll_event events[1024];

  int nfds = epoll_wait(epoll_fd, events, 1024, -1);
  if (nfds == -1) {
    throw std::runtime_error(strerror(errno));
  }

  for (int i = 0; i < nfds; i++) {
    int current_fd = events[i].data.fd;

    if (current_fd == listen_fd) {
      sockaddr_storage client{};
      socklen_t len = sizeof(client);
      shared_fd connection_fd = accept(listen_fd, (sockaddr *) &client, &len);
      address client_addr(client);

      shared_fd event_fd = eventfd(0, 0);

      struct epoll_event end_event = {.events = EPOLLIN | EPOLLET, .data = {.fd = event_fd}};
      epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event_fd, &end_event);

      conn_fds[event_fd] = event_fd;
      connections[event_fd] = std::thread(&base_server::routine_wrapper, this, client_addr, connection_fd, event_fd);
    } else {
      connections[current_fd].join();
      connections.erase(current_fd);
      conn_fds.erase((int) current_fd);
      epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, nullptr);
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
    count[client.get_str()]++;
    timeouts[client.get_str()] = resource / count[client.get_str()];
    connection_routine(connection_fd, event_fd, timeouts[client.get_str()]);
  } catch (std::runtime_error &e) {
    log.err("{" + std::to_string(connection_fd) + "}: " + e.what());
  }

  count[client.get_str()]--;
  if (count[client.get_str()] == 0) {
    count.erase(client.get_str());
    timeouts.erase(client.get_str());
  } else {
    timeouts[client.get_str()] = resource / count[client.get_str()];
  }

  log.log("Disconnected {" + std::to_string(connection_fd) + "}: " + client.get_full_str());
}

void base_server::connection_routine(const shared_fd &connection_fd, const shared_fd &event_fd, std::time_t &timeout) {
  std::time_t start_time = std::time(nullptr);

  shared_fd efd = epoll_create1(0);

  struct epoll_event conn = {.events = EPOLLIN, .data = {.fd = connection_fd}};
  struct epoll_event cancel = {.events = EPOLLIN | EPOLLET, .data = {.fd = event_fd}};

  epoll_ctl(efd, EPOLL_CTL_ADD, connection_fd, &conn);
  epoll_ctl(efd, EPOLL_CTL_ADD, event_fd, &cancel);

  struct epoll_event events[1024];
  bool quit = false;
  std::string buffer;

  while (!quit) {
    std::time_t now = std::time(nullptr);
    int current_timeout = std::max(std::time_t(0), timeout - (now - start_time) * 1000);

    int nfds = epoll_wait(efd, events, 1024, current_timeout);
    if (nfds == -1) {
      throw std::runtime_error(strerror(errno));
    }

    if (nfds == 0) {
      break;
    }

    for (int i = 0; i < nfds; i++) {
      int fd = events[i].data.fd;
      if (fd == connection_fd) {
        int rv = data_handler(connection_fd, buffer);
        if (rv == 0) quit = true;
      } else if (fd == event_fd) {
        eventfd_t val;
        eventfd_read(fd, &val);
        quit = (val == 2);
      }
    }
  }
  eventfd_write(event_fd, 1);
}
