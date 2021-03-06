#ifndef server_h
#define server_h

#include <vector>
#include <string>

#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in

using namespace std;

class Server {
public:
  Server();
  ~Server();

  void startListening();

private:
  struct Request{
    string filePath;
    string exten;
  };

  void createSocket();
  struct Request parseMessage(string s);
  char * respond(struct Request req, int &resplen);

  int sockfd;
  struct sockaddr_in serv_addr;

  int n_cli;
  int cli_fd;
  struct sockaddr_in cli_addr;


  // for asynchronous connections
  //vector<int> cli_fd;
  //vector<struct sockaddr_in> cli_addrs;




};

#endif
