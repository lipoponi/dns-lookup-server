#include <iostream>
#include <memory>

#include "app.h"
#include "logger.h"

int main() {
  logger console(STDOUT_FILENO, STDERR_FILENO);

  try {
    app serv(console);
    serv.setup("127.0.0.1", 1337);
    serv.run();
  } catch (std::runtime_error &e) {
    console.err(e.what());
    return 1;
  } catch (...) {
    return 1;
  }

  return 0;
}