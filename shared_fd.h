#ifndef SHARED_FD_H
#define SHARED_FD_H

#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <unistd.h>

class shared_fd {
  int *counter;
  int fd;

 public:
  shared_fd(int fd = -1);
  shared_fd(const shared_fd &other);
  shared_fd& operator=(const shared_fd &other);
  ~shared_fd();
  operator int() const;
};

#endif //SHARED_FD_H
