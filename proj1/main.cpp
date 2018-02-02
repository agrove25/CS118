#include "server.h"

int main() {
  Server serv = Server();
  serv.startListening();

  return 0;
}
