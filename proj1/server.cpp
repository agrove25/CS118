#define portno 9009

#include "server.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <cstring>
#include <climits>


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

  //cout << "Server initialized..." << endl << endl;
}

Server::~Server() {
  //cout << "Closing..." << endl;

  close(cli_fd);
  close(sockfd);
}

void Server::createSocket() {
  //cout << "Socket created..." << endl;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);  // create socket
  if (sockfd < 0)
      cerr << "ERROR opening socket" << endl;

  memset((char *) &serv_addr, 0, sizeof(serv_addr));   // reset memory

  // fill in address info
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  // attach this address info to the socket
  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    cerr << "ERROR on binding: " << strerror(errno) <<  endl;

}

void Server::startListening() {
  listen(sockfd, 5);  // 5 simultaneous connection at most

  socklen_t clilen;

  while (1) {
    // establish a connection and assign it a fd
    cli_fd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    int n;
    char buffer[2048];

    memset(buffer, 0, 2048);  // reset memory

    //read client's message
    n = read(cli_fd, buffer, 2048);
    if (n < 0){ cerr << "ERROR reading from socket: " << strerror(errno) << endl; return;}
    printf("%s", buffer);
    string collected(buffer);
    
    Request req = parseMessage(collected);
    int resplen = 0;
    char *response = respond(req, resplen);

    //reply to client
    n = write(cli_fd, response, resplen);
    if (n < 0) cerr << "ERROR writing to socket" << endl;

    delete response;

    close(cli_fd);  // close connection
  }

  close(sockfd);  // close socket (also handled in destructor)

}

struct Server::Request Server::parseMessage(string buffer) {
  struct Request req;

  //cout << "Attempting to parse message..." << endl;

  vector<string> lines;
  string line = "";

  // break request into lines by /r/n
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

  /*for (int i = 0; i < lines.size(); i++) {
    cout << lines[i] << endl;
  }*/

  if (lines[0].substr(0, 3) == "GET") {
    //cout << "GETting" << endl;

    // extract the requested file, stripping away the HTTP version and the GET
    req.filePath = lines[0].substr(4, lines[0].length() - 13);
    //cout << "this: " << req.filePath << endl;

    // we'll need to use ./filename, so we keep the initial slash.
    string newfp = ".";
    newfp += req.filePath;

    // find and replace all instances of %20 with a space
    int space;
    while((space = newfp.find("%20")) != string::npos){
      newfp = newfp.replace(space, 3, " ");
    }

    // extract the file extension, which will be used to establish content type
    int dot = newfp.find_last_of(".");
    req.exten = "";
    for(int i = dot + 1; i < newfp.length(); i++){
      newfp[i] = tolower(newfp[i]); // make the extension lowercase so it's
      req.exten += newfp[i];        // not case-sensitive
    }

    req.filePath = newfp;
    //cout << "Path: " << req.filePath << endl;


  }

  //cout << "Parsed!" << endl;

  return req;
}

char * Server::respond(struct Request req, int &resplen) {
  //cout << "Responding..." << endl;
  //cout << req.filePath << endl;

  ostringstream oss;

  string status = "200 OK";
  int len = 0;

  // attempt to open the file requested
  fstream file;
  file.open(req.filePath.c_str());
  //cout << "Opening file..." << endl;
  char *body = new char[0];
  if(file.fail()){
    // It will fail if the file does not exist, triggering a 404
    //cout << "No file" << endl;
    status = "404 Not Found";
    delete body;
    body = new char[36];
    strcpy(body, "<html><h1>404 Not Found</h1></html>");
    len = 36;
  }
  else{
    //cout << "File found" << endl;
    file.ignore(INT_MAX); // skip to the end of the file
    len = file.gcount();  // see how far we skipped, that's the file's length
    file.clear();
    file.seekg(ios_base::beg);
    delete body;
    body = new char[len]; // read the whole file
    file.read(body, len);
  }

  // the content type depends on the file extension found earlier
  string type = "octet-stream";
  if(req.exten == "html" || req.exten == "htm"){
    type = "text/html";
  }
  else if(req.exten == "jpg" || req.exten == "jpeg" || req.exten == "gif"){
    type = "image";
  }

  // The only headers needed are these
  oss << "Content-Length: " << len << "\r\nContent-Type: " << type << "\r\n";
  string headers = oss.str();

  // the status line and headers are combined here
  // Notice that these use C++ strings because it is easy, but the file is
  // read into a C string to ensure all bytes are preserved
  oss.str("");
  oss << "HTTP/1.1 " << status << "\r\n" << headers << "\r\n";
  string resphead = oss.str();

  //cout << "resphead:\n" << resphead << endl;

  // combine the header and body into one C string
  char *response = new char[resphead.length() + len + 1];
  strcpy(response, resphead.c_str());
  memcpy(response + resphead.length(), body, len);
  delete body; // the contents have been copied out, so this is not needed
  resplen = resphead.length() + len; // there may be 0 bytes in a binary file,
                                     // so we need to keep track of length
                                     // rather than using strlen
  //printf("Response:\n%sEnd\n", response);
  return response;
}
