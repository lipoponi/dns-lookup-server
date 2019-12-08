#include <iostream>
#include <string>

#include "base_server.h"
#include "gai_server.h"

int main() {
  BaseServer *server = new GaiServer();
  server->setup("localhost", "15213");
  while (true) {
    int rv = server->exec();
    if (rv == -1) break;
  }
  return 0;
}