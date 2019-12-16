#include <iostream>
#include <memory>

#include "base_server.h"
#include "gai_server.h"

int main() {
  try {
    std::unique_ptr<base_server> server = std::make_unique<gai_server>();
    server->setup("127.0.0.1", 1337);
    server->loop();
  } catch (std::runtime_error &e) {
    std::cerr << "[FATAL] " << e.what() << std::endl;
    return 1;
  } catch (...) {
    return 1;
  }

  return 0;
}