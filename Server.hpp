#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
// #include <pthread.h>
#include "Database.hpp"
#include "pugixml/pugiconfig.hpp"
#include "pugixml/pugixml.hpp"
#include "threadpool/cptl.h"
#include "threadpool/threadpool.h"

#define MAX_TCP_PACKET_SIZE 65535

class Server {
 private:
  int hasError;
  int socket_fd;
  int client_connection_fd;
  const int port_num;
  struct addrinfo host_info;
  struct addrinfo * host_info_list;

 public:
  explicit Server(int port_num);
  void startRun();

  // create a socket to listen
  int setUpStruct();
  int initSocketFd();
  int tryBind();
  int startListen();

  // try accept
  int tryAccept();
  // send and recv data
  std::string recvData(int flag);
  static bool sendAllData(int sockfd, const char * msg, size_t size);
  int getErrorSign();

  // handle request
  static void handleRequest(const int & client_connection_fd);
  static pugi::xml_document process_create(const pugi::xml_document & doc,
                                           const int & client_connection_fd,
                                           Database db);
  static pugi::xml_document process_transactions(const pugi::xml_document & doc,
                                                 const int & client_connection_fd,
                                                 Database db);

  // int connectToServer();
};
