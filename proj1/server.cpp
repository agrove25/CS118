#define portno 8080

#include "server.h"

#include <iostream>
#include <cstring>


#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in


using namespace std;


Server::Server() {
  createSocket();

  n_cli = 0;

  cout << "Server Initialized..." << endl;

}

Server::~Server() {

}

void Server::startListening() {
  listen(sockfd, 5);  // 5 simultaneous connection at most

  socklen_t clilen;    // not entirely sure what this for..
  cli_fd.resize(n_cli + 1); // creating room for more clients
  cli_fd[n_cli] = accept(sockfd, (struct sockaddr *) &cli_addrs[n_cli], &clilen);
  n_cli++;

  // TODO: ASYNCHRONOUS IO USING SELECT? OR POLL? OR FORK?

  if (cli_fd[0] < 0)
   cerr << "ERROR on accept" << endl;

  int n;
  char buffer[256];

  memset(buffer, 0, 256);  // reset memory

  //read client's message
  n = read(cli_fd[0], buffer, 255);
  if (n < 0) cerr << "ERROR reading from socket" << endl;
  printf("Here is the message: %s\n", buffer);

  //reply to client
  n = write(cli_fd[0], "I got your message", 18);
  if (n < 0) cerr << "ERROR writing to socket" << endl;

  close(cli_fd[0]);  // close connection
  close(sockfd);
}

void Server::createSocket() {
  cout << "Socket created..." << endl;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);  // create socket
  if (sockfd < 0)
      cerr << "ERROR opening socket" << endl;

  memset((char *) &serv_addr, 0, sizeof(serv_addr));   // reset memory

  // fill in address info
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
      cerr << "ERROR on binding" << endl;


}
