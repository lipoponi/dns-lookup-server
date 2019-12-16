#include "logger.h"

logger::logger(int log_fd, int err_fd)
    : log_fd(log_fd), err_fd(err_fd) {}

void logger::log(std::string msg) {
  std::time_t now = std::time(nullptr);
  msg = std::to_string(now) + " [LOG] " + msg + "\n";
  write(log_fd, msg.c_str(), msg.size());
}

void logger::err(std::string msg) {
  std::time_t now = std::time(nullptr);
  msg = std::to_string(now) + " [ERROR] " + msg + "\n";
  write(err_fd, msg.c_str(), msg.size());
}
