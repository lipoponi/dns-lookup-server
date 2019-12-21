#include "logger.h"

logger::logger(int log_fd, int err_fd)
    : log_fd(log_fd), err_fd(err_fd) {}

void logger::log(std::string msg) {
  std::time_t now = std::time(nullptr);
  msg = std::to_string(now) + " [LOG] " + msg + "\n";
  if (write(log_fd, msg.c_str(), msg.size()) == -1) {
    throw std::runtime_error(strerror(errno));
  }
}

void logger::err(std::string msg) {
  std::time_t now = std::time(nullptr);
  msg = std::to_string(now) + " [ERROR] " + msg + "\n";
  if (write(err_fd, msg.c_str(), msg.size()) == -1) {
    throw std::runtime_error(strerror(errno));
  }
}
