#include <iostream>
#include <memory>

#include "base_server.h"
#include "gai_server.h"
#include "logger.h"

int main() {
  logger console(STDOUT_FILENO, STDERR_FILENO);

  try {
    std::unique_ptr<base_server> server = std::make_unique<gai_server>(console);
    server->setup("127.0.0.1", 1337);
    server->loop();
  } catch (std::runtime_error &e) {
    console.err(e.what());
    return 1;
  } catch (...) {
    return 1;
  }

  return 0;
}