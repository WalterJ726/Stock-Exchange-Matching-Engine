#include <cstdio>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <cstring>
#include <string>
#include <stdlib.h>
#include <string.h>
// #include <pthread.h>
#include "threadpool/cptl.h"
#include "threadpool/threadpool.h"
#include "pugixml/pugiconfig.hpp"
#include "pugixml/pugixml.hpp"
#include "Database.hpp"

#define MAX_TCP_PACKET_SIZE 65535

class Server{
private:
    int hasError;
    int socket_fd;
    int client_connection_fd;
    const int port_num;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;

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
    void sendData(void* data, size_t dataSize, int flag);
    int getErrorSign();


    // handle request
    static void handleRequest(int client_connection_fd, const Database& db);
    // int connectToServer();
};