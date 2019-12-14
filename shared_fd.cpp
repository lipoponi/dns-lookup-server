#include "shared_fd.h"

shared_fd::shared_fd(int fd)
    : counter(new int(1)), fd(fd) {
  assert(fd == -1 || fcntl(fd, F_GETFD) != -1);
}

shared_fd::shared_fd(const shared_fd &other)
    : counter(other.counter), fd(other.fd) {
  (*counter)++;
}

shared_fd &shared_fd::operator=(const shared_fd &other) {
  if (this != &other) {
    (*counter)--;

    if (*counter == 0) {
      delete counter;
      if (fd != -1) {
        close(fd);
      }
    }

    counter = other.counter;
    fd = other.fd;
    (*counter)++;
  }

  return *this;
}

shared_fd::~shared_fd() {
  (*counter)--;
  if (*counter == 0) {
    delete counter;
    if (fd != -1) {
      close(fd);
    }
  }
}

shared_fd::operator int() const {
  return fd;
}