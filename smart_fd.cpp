#include <iostream>
#include <cstring>
#include "smart_fd.h"

shared_fd::shared_fd(int fd)
    : data(new block) {
  assert(fd == -1 || fcntl(fd, F_GETFD) != -1);
  data->counter = 1;
  data->fd = fd;
}

shared_fd::shared_fd(const shared_fd &other)
    : data(other.data) {
  data->counter++;
}

shared_fd &shared_fd::operator=(const shared_fd &other) {
  if (this != &other) {
    data->counter--;

    if (data->counter == 0) {
      if (data->fd != -1 && close(data->fd) == -1) {
        throw std::runtime_error(std::to_string(data->fd) + " " + strerror(errno));
      }
      delete data;
    }

    data = other.data;
    data->counter++;
  }

  return *this;
}

shared_fd::~shared_fd() {
  data->counter--;
  if (data->counter == 0) {
    if (data->fd != -1 && close(data->fd) == -1) {
      throw std::runtime_error(std::to_string(data->fd) + " " + strerror(errno));
    }
    delete data;
  }
}

shared_fd::operator int() const {
  return data->fd;
}
