#include <iostream>
#include <string>

#include "base_server.h"
#include "gai_server.h"

int main() {
  BaseServer *server = new GaiServer();
  int rv = server->setup("localhost", "15213");
  while (rv != -1) {
    rv = server->exec();
  }

  return 0;
}