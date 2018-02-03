#define portno 9000

#include "server.h"

#include <iostream>
#include <sstream>
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

  cout << "Server initialized..." << endl << endl;
}

Server::~Server() {
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

void Server::startListening() {
  listen(sockfd, 5);  // 5 simultaneous connection at most

  socklen_t clilen;    // not entirely sure what this for..

  while (1) {
    cli_fd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    int n;
    ostringstream collector;
    //string collected;
    char buffer[2048];

    //do{
      memset(buffer, 0, 2048);  // reset memory

      //read client's message
      n = read(cli_fd, buffer, 2048);
      if (n < 0) cerr << "ERROR reading from socket" << endl;
      printf("%s", buffer);
      //collector << buffer;
      //collected = collector.str();
      //} while(collected.length() > 0 && collected.substr(collected.length() - 4) != "\r\n\r\n");
      string collected(buffer);
    
    Request req = parseMessage(collected);
    const char *response = respond(req).c_str();

    //reply to client
    //n = write(cli_fd, "I got your message\n", 19);
    n = write(cli_fd, response, strlen(response));
    if (n < 0) cerr << "ERROR writing to socket" << endl;

    shutdown(cli_fd, SHUT_WR);  // close connection
  }

  close(sockfd);

  /* ASYNCHRNOUS?
  socklen_t clilen;    // not entirely sure what this for..
  cli_fd.resize(n_cli + 1); // creating room for more clients
  cli_fd[n_cli] = accept(sockfd, (struct sockaddr *) &cli_addrs[n_cli], &clilen);
  n_cli++;

  // TODO: ASYNCHRONOUS IO USING SELECT? OR POLL? OR FORK?

  if (cli_fd[0] < 0)
   cerr << "ERROR on accept" << endl;

    int n;
    char buffer[2048];

    memset(buffer, 0, 2048);  // reset memory

    //read client's message
    n = read(cli_fd[0], buffer, 2048);
    if (n < 0) cerr << "ERROR reading from socket" << endl;
    printf("%s", buffer);

    //reply to client
    n = write(cli_fd[0], "I got your message\n", 19);
    if (n < 0) cerr << "ERROR writing to socket" << endl;

    close(cli_fd[0]);  // close connection
  }
    */

}

struct Server::Request Server::parseMessage(string buffer) {
  struct Request req;

  cout << "Attempting to parse message..." << endl;

  vector<string> lines;
  string line = "";

  for (int i = 0; i < 2048; i++) {
    char c = buffer[i];

    if (c == '\r') {
      if(line == ""){
	break;
      }
      lines.push_back(line);
      line = "";
    }
    else if (c != '\0' && c != '\r' && c != '\n') {
      line += c;
    }
    else if (c == '\0'){
      break;
    }
  }

  for (int i = 0; i < lines.size(); i++) {
    cout << lines[i] << endl;
  }

  if (lines[0].substr(0, 3) == "GET") {
    req.filePath = lines[0].substr(4, lines[0].length() - 9);

    // TODO: CASE INSENSITIVITY?

    string newfp = "";
    int space;
    
    while((space = req.filePath.find("%20")) != string::npos){
      newfp = req.filePath.replace(space, 3, " ");
    }
    
    /*for (int i = 0; i < req.filePath.length(); i++) {
      if (i < req.filePath.length() - 2 && req.filePath[i] == '%' && req.filePath[i + 1] == '2' && req.filePath[i + 2] == '0') {
        newfp += " ";
        i += 2;
        continue;
      }

      newfp += req.filePath[i];
    }*/

    req.filePath = newfp;
    cout << req.filePath << endl;


  }

  return req;
}

string Server::respond(struct Request req) {
  cout << req.filePath << endl;
  int len = 0;
  FILE *file = fopen(req.filePath.c_str(), "r");
  if(file == NULL){
    len = 5;
  }
  else{
    fclose(file);
  }

  ostringstream oss;

  string status = "200 OK";//"404 Not Found";
  oss << "Connection: close\r\n" << "Content-Length: " << len << /*"\r\nContent-Encoding:*/"\r\nContent-Type: text/html\r\n";
  string headers = oss.str();
  string body = "aaaa";
  oss.str("");
  oss << "HTTP/1.0 " << status << "\r\n" << headers << "\r\n" << body << "";
  string response = oss.str();

  cout << response << "End\n";
  return response;
}
