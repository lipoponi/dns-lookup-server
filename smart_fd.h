#ifndef SMART_FD_H
#define SMART_FD_H

#include <unistd.h>
#include <cassert>
#include <fcntl.h>

class shared_fd;
class weak_fd;

class shared_fd {
  int *counter;
  int fd;

 public:
  shared_fd(int fd = -1);
  shared_fd(const shared_fd &other);
  shared_fd &operator=(const shared_fd &other);
  ~shared_fd();
  operator int() const;

  friend class weak_fd;
};

class weak_fd {
  int *counter;
  int fd;

 public:
  weak_fd();
  weak_fd(const shared_fd &shared);
  operator int() const;
  [[nodiscard]] int use_count() const;
  [[nodiscard]] bool expired() const;
};

#endif //SMART_FD_H
