#ifndef SMART_FD_H
#define SMART_FD_H

#include <unistd.h>
#include <cassert>
#include <fcntl.h>

class shared_fd;
class weak_fd;

class shared_fd {
  struct block {
    int counter;
    int fd;
  } *data;

 public:
  shared_fd(int fd = -1);
  shared_fd(const shared_fd &other);
  shared_fd &operator=(const shared_fd &other);
  ~shared_fd();
  operator int() const;

  friend class weak_fd;
};

#endif //SMART_FD_H
