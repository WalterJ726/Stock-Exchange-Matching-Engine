#include "Server.hpp"

void Server::startRun(){
    std::cout << "start Run server" << std::endl;
    
    // start getting order from client
    while (true){
        int client_connection_fd = server.tryAccept();
        
    }
}