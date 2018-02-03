#include "server.h"
#include <stdlib.h>
#include <signal.h>

void *servpoint;

void close_server(int sig){
  delete (Server *)servpoint;
  exit(0);
}

int main() {
  Server *serv = new Server();
  servpoint = serv;
  signal(SIGINT, close_server);
  serv->startListening();
  delete serv;
  return 0;
}
