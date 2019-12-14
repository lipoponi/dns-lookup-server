#include <iostream>
#include <string>

#include "base_server.h"
#include "gai_server.h"

int main() {
  try {
    base_server *server = new gai_server();
    server->setup();
    server->loop();
  } catch (std::runtime_error &e) {
    std::cerr << e.what() << std::endl;
  } catch (...) {
    return 1;
  }

  return 0;
}