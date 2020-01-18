#ifndef LOGGER_H
#define LOGGER_H

#include <cstring>
#include <ctime>
#include <memory>
#include <string>
#include <unistd.h>

class logger {
  int log_fd;
  int err_fd;

 public:
  explicit logger(int log_fd = STDOUT_FILENO, int err_fd = STDERR_FILENO);

  void log(std::string msg);
  void err(std::string msg);
};

#endif //LOGGER_H
