#ifndef UNIQUE_FD_H
#define UNIQUE_FD_H

#include <algorithm>
#include <unistd.h>

class unique_fd {
  int fd_ = -1;

 public:
  unique_fd() noexcept = default;
  explicit unique_fd(int fd) noexcept;
  ~unique_fd();

  unique_fd(unique_fd const &) = delete;
  unique_fd &operator=(unique_fd const &) = delete;

  unique_fd(unique_fd &&) noexcept;
  unique_fd &operator=(unique_fd &&) noexcept;

  [[nodiscard]] int fd() const noexcept;

  friend void swap(unique_fd &, unique_fd &) noexcept;
};

#endif //UNIQUE_FD_H
