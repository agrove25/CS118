#define portno 9008

#include "server.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>


#include <stdio.h>
#include <unistd.h>
#include <errno.h>
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
  cout << "Closing..." << endl;

  close(cli_fd);
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
    cerr << "ERROR on binding: " << strerror(errno) <<  endl;

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
    if (n < 0){ cerr << "ERROR reading from socket: " << strerror(errno) << endl; return;}
    printf("%s", buffer);
    //collector << buffer;
    //collected = collector.str();
    //} while(collected.length() > 0 && collected.substr(collected.length() - 4) != "\r\n\r\n");
    string collected(buffer);
    
    Request req = parseMessage(collected);
    int resplen = 0;
    char *response = respond(req, resplen);

    //reply to client
    //n = write(cli_fd, "I got your message\n", 19);
    n = write(cli_fd, response, resplen);
    if (n < 0) cerr << "ERROR writing to socket" << endl;

    delete response;

    //shutdown(cli_fd, SHUT_WR);  // close connection
    close(cli_fd);
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
    cout << "GETting" << endl;
    req.filePath = lines[0].substr(4, lines[0].length() - 13);
    cout << "this: " << req.filePath << endl;

    string newfp = ".";
    newfp += req.filePath;
    int space;
    
    while((space = newfp.find("%20")) != string::npos){
      newfp = newfp.replace(space, 3, " ");
    }
    int dot = newfp.find_last_of(".");
    req.exten = "";
    for(int i = dot + 1; i < newfp.length(); i++){
      newfp[i] = tolower(newfp[i]);
      req.exten += newfp[i];
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
    cout << "Path: " << req.filePath << endl;


  }

  cout << "Parsed!" << endl;

  return req;
}

char * Server::respond(struct Request req, int &resplen) {
  cout << "Responding..." << endl;
  cout << req.filePath << endl;

  ostringstream oss;

  string status = "200 OK";
  int len = 0;
  fstream file;
  file.open(req.filePath.c_str());
  cout << "Opening file..." << endl;
  char *body = new char[0];
  if(file.fail()){
    cout << "No file" << endl;
    status = "404 Not Found";
  }
  else{
    cout << "File found" << endl;
    char *bodbuf = new char[1024];
    char *acc = new char[1024];
    while(!file.fail()){
      file.read(bodbuf, 1024);
      char *newacc = new char[len + 1024];
      memcpy(newacc, acc, len);
      delete [] acc;
      acc = newacc;
      memcpy(acc + len, bodbuf, 1024);
      
      len += 1024;
    }
    delete [] bodbuf;
    len -= 1024;
    len += file.gcount();
    body = new char[len];
    memcpy(body, acc, len);
    file.close();
  }

  string type = "octet-stream";
  if(req.exten == "html" || req.exten == "htm"){
    type = "text/html";
  }
  else if(req.exten == "jpg" || req.exten == "jpeg" || req.exten == "gif"){
    type = "image";
  }
  oss << "Content-Length: " << len << "\r\nContent-Type: " << type << "\r\n";
  string headers = oss.str();

  oss.str("");
  oss << "HTTP/1.1 " << status << "\r\n" << headers << "\r\n";
  string resphead = oss.str();

  cout << "resphead:\n" << resphead << endl;

  char *response = new char[resphead.length() + len + 1];
  strcpy(response, resphead.c_str());
  memcpy(response + resphead.length(), body, len);
  resplen = resphead.length() + len;
  printf("Response:\n%sEnd\n", response);
  return response;
}
