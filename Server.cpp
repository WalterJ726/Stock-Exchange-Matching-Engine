#include "Server.hpp"

void Server::startRun(){
    std::cout << "start Run server" << std::endl;
    std::threadpool executor{50};
    Database db("exchange", "postgres", "passw0rd");
    db.connect();
    db.initialize();
    // start getting order from client
    while (true){
        int client_connection_fd = tryAccept();
        if (client_connection_fd == -1){
            std::cout << "accpet failed" << std::endl;
            continue;
        }
        executor.commit(handleRequest, client_connection_fd, db).get();
    }

}

void Server::handleRequest(int client_connection_fd, const Database& db){
    std::vector<char> buff(MAX_TCP_PACKET_SIZE);
    // recv data
    size_t numbytes = recv(client_connection_fd, &buff.data()[0], MAX_TCP_PACKET_SIZE, 0);

    // get length of xml
    std::string xml_len;
    for (size_t i = 0; i < buff.size(); i ++ ){
        if (isdigit(buff[i])){
            xml_len += buff[i];
        } else {
            break;
        }
    }

    size_t xml_tot;
    try
    {
        xml_tot = std::stoul(xml_len, nullptr, 0);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        std::cout << "fail to convert number" << std::endl;
    }
    
    // continue to recv data
    size_t cur_bytes = numbytes - xml_len.size() - 1;
    size_t len = 0;
    while (cur_bytes < xml_tot){
        buff.resize(buff.size() + 512);
        if ((len = (recv(client_connection_fd, &buff.data()[buff.size()], 512, 0))) <= 0)
            break;
        buff.resize(buff.size() + len);
        cur_bytes += len; 
    }
    

    // covert to xml, start to process
    std::string xml_str(buff.begin() + xml_len.size() + 1, buff.end());
    pugi::xml_document doc;
    std::cout << xml_str << std::endl;
    // Load the XML string into the document object
    if (!doc.load_string(xml_str.c_str()))
    {
        std::cerr << "Failed to load XML string." << std::endl;
    }
    
    // Access the root element of the XML document
    pugi::xml_node root = doc.document_element();
    std::string root_name = root.name();

    if (root_name == "create"){
        std::cout << "start to process create" << std::endl;
    } else if (root_name == "transactions"){
        std::cout << "start to process transactions" << std::endl;

    } else {
        std::cout << "wrong xml" << std::endl;
    }
    close(client_connection_fd);
}


Server::Server(int port_num) : port_num(port_num) {
  hasError = 0;
  if (setUpStruct() == -1){
    hasError = 1;
  }
  if (initSocketFd() == -1){
    hasError = 1;
  }
  if (tryBind() == -1){
    hasError = 1;
  }
  if (startListen() == -1){
    hasError = 1;
  }
}

int Server::setUpStruct(){
  int status;
  memset(&host_info, 0, sizeof(host_info));

  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags    = AI_PASSIVE;
  std::string port_str = std::to_string(port_num);
  status = getaddrinfo(nullptr, port_str.c_str(), &host_info, &host_info_list);
  if (status != 0) {
    // "Error: cannot get address info for host"
    return -1;
  }
  return status;
}

int Server::initSocketFd(){
  socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    // "Error: cannot create socket" << endl;
    return -1;
  }
  return socket_fd;
}

int Server::tryBind(){
  int status;
  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    // "Error: cannot bind socket" << endl;
    return -1;
  }
  return status;
}

int Server::startListen(){
    int status;
    status = listen(socket_fd, 100);
  if (status == -1) {
    // "Error: cannot listen on socket" 
    return -1;
  }
  freeaddrinfo(host_info_list);
  return status;
}


int Server::tryAccept(){
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    client_connection_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (client_connection_fd == -1) {
      // "Error: cannot accept connection on socket"
      return -1;
    }
    return client_connection_fd;
}

int Server::getErrorSign(){
  return hasError;
}

std::string Server::recvData(int flag){
  char recvbuff[MAX_TCP_PACKET_SIZE];
  int numbytes;
  if ((numbytes = recv(client_connection_fd, recvbuff, MAX_TCP_PACKET_SIZE, flag)) == -1) {
      hasError = 1;
      return nullptr;
  }
  // recvbuff[numbytes] = '\0';
  return std::string(recvbuff, numbytes);
}

void Server::sendData(void* data, size_t dataSize, int flag){
  int status;
  status = send(client_connection_fd, data, dataSize, MSG_NOSIGNAL); 
  if (status == -1){
    hasError = 1;
  }
}


