#include "unique_fd.h"

unique_fd::unique_fd(int fd) noexcept
    : fd_(fd) {}

unique_fd::~unique_fd() {
  if (fd_ != -1)
    close(fd_);
}

unique_fd::unique_fd(unique_fd &&rhs) noexcept {
  swap(*this, rhs);
}

unique_fd &unique_fd::operator=(unique_fd &&rhs) noexcept {
  swap(*this, rhs);
  return *this;
}

int unique_fd::fd() const noexcept {
  return fd_;
}

void swap(unique_fd &a, unique_fd &b) noexcept {
  std::swap(a.fd_, b.fd_);
}
