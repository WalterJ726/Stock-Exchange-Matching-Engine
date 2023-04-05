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

void Server::handleRequest(const int& client_connection_fd, Database db){
    std::vector<char> buff(MAX_TCP_PACKET_SIZE);
    // recv data
    size_t numbytes = recv(client_connection_fd, &buff.data()[0], MAX_TCP_PACKET_SIZE, 0);

    // get length of xml
    std::string xml_len;
    for (size_t i = 0; i < buff.size(); i ++ ){
        if (buff[i] == '\n'){
            break;
        } else {
            xml_len += buff[i];
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

    pugi::xml_document response_raw;
    if (root_name == "create"){
        std::cout << "start to process create" << std::endl;
        response_raw = process_create(doc, client_connection_fd, db);
    } else if (root_name == "transactions"){
        std::cout << "start to process transactions" << std::endl;
        response_raw = process_transactions(doc, client_connection_fd, db);
    } else {
        std::cout << "wrong xml" << std::endl;
    }
    // send reponse back
    response_raw.print(std::cout);
    std::ostringstream oss;
    response_raw.save(oss);
    std::string response = oss.str();
    sendAllData(client_connection_fd, response.c_str(), response.size());
    close(client_connection_fd);
}

pugi::xml_document Server::process_create(const pugi::xml_document& doc, const int& client_connection_fd, Database db){
    pugi::xml_document response;
    pugi::xml_node decl = response.append_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";
    pugi::xml_node result_node = response.append_child("results");
    pugi::xml_node root = doc.document_element();
    for (pugi::xml_node cur = root.first_child(); cur; cur = cur.next_sibling()){
      if (std::string(cur.name()) == "account"){
            // TODO: id and balance is not valid and add error msg
            std::cout << "start to process creating account" << std::endl;
            std::string account_id = cur.attribute("id").value();
            std::string balance_raw = cur.attribute("balance").value();
            size_t balance = std::stoul(balance_raw);
            
            if (db.insert_account(account_id, balance)){
                pugi::xml_node created_account_child = result_node.append_child("created");
                created_account_child.append_attribute("id") = account_id.c_str(); 
            } else {
                pugi::xml_node invalid_account_child = result_node.append_child("error");
                invalid_account_child.append_attribute("id") = account_id.c_str();
                invalid_account_child.append_child(pugi::node_pcdata).set_value("Account already exists");
            }
      } else if (std::string(cur.name()) == "symbol"){
            // TODO: sym is not valid and add error msg
            // TODO: the indent of error msg
            std::cout << "start to process creating symbol" << std::endl;
            std::string sym = cur.attribute("sym").value();
            for (pugi::xml_node sym_create = cur.first_child(); sym_create; sym_create = sym_create.next_sibling()){
                std::string account_id = sym_create.attribute("id").value();
                std::string num_raw = sym_create.child_value();
                size_t num = std::stoul(num_raw);
                if (db.insert_sym(account_id, sym, num)){
                  pugi::xml_node created_account_child = result_node.append_child("created");
                  created_account_child.append_attribute("sym") = sym.c_str(); 
                  created_account_child.append_attribute("id") = account_id.c_str(); 
                } else {
                  // TODO: Will this function produce error?
                  std::cout << "Will this function produce error" << std::endl;
                }
            }
      } else {
        pugi::xml_node error_child = result_node.append_child("error");
        error_child.append_child(pugi::node_pcdata).set_value("invalid child name");
        std::cout << "bad child in create" << std::endl;
        // throw std::exception();
      }
  }
  return response;
  // TODO: send response back
}

pugi::xml_document Server::process_transactions(const pugi::xml_document& doc, const int& client_connection_fd, Database db){
    pugi::xml_document response;
    pugi::xml_node decl = response.append_child(pugi::node_declaration);
    decl.append_attribute("version") = "1.0";
    decl.append_attribute("encoding") = "UTF-8";
    pugi::xml_node result_node = response.append_child("results");
    pugi::xml_node root = doc.document_element();
    std::string account_id = root.attribute("id").value();
    bool isAccountValid = db.find_account(account_id);
    for (pugi::xml_node cur = root.first_child(); cur; cur = cur.next_sibling()){
        if (!isAccountValid){
            pugi::xml_node invalid_account = result_node.append_child("error");
            invalid_account.append_attribute("id") = account_id.c_str();
            invalid_account.append_child(pugi::node_pcdata).set_value("Account ID doesn't exist");
            continue;
        }
        if (std::string(cur.name()) == "query"){
          std::cout << "start to query" << std::endl;

        } else if (std::string(cur.name()) == "cancel"){
          std::cout << "start to cancel" << std::endl;

        } else if (std::string(cur.name()) == "order"){
          std::cout << "start to order" << std::endl;
          // TODO: sym amount limit invalid (does not exist, name is wrong)
          std::string sym = cur.attribute("sym").value();
          std::string amount_raw = cur.attribute("amount").value();
          int amount = std::stoi(amount_raw);
          std::string limit_raw = cur.attribute("limit").value();
          double limit = std::stod(limit_raw);
          size_t trans_id;
          if(db.insert_order(account_id, sym, amount, limit, trans_id)){
            pugi::xml_node open_order_child = result_node.append_child("opened");
            open_order_child.append_attribute("sym") = sym.c_str(); 
            open_order_child.append_attribute("amount") = amount_raw.c_str(); 
            open_order_child.append_attribute("limit") = limit_raw.c_str();
            open_order_child.append_attribute("id") = std::to_string(trans_id).c_str();  
          } else {
            pugi::xml_node invalid_insert_child = result_node.append_child("error");
            invalid_insert_child.append_attribute("sym") = sym.c_str();
            invalid_insert_child.append_attribute("amount") = amount_raw.c_str();
            invalid_insert_child.append_attribute("limit") = limit_raw.c_str();
            invalid_insert_child.append_child(pugi::node_pcdata).set_value("Amount or balance invalid");
          }
          // execute an order
          db.executed_order(account_id, sym, amount, limit, trans_id);
        } else {
            pugi::xml_node error_child = result_node.append_child("error");
            error_child.append_child(pugi::node_pcdata).set_value("invalid child name");
            std::cout << "bad child in transactions" << std::endl;
        }
    }
    return response;
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

bool Server::sendAllData(int sockfd, const char *msg, size_t size){
  size_t numBytes = 0;
  size_t bytesleft = size;
  int recvBytes = 0;
  while ((numBytes < bytesleft))
  {
      if ((recvBytes = send(sockfd, msg + numBytes, size, MSG_NOSIGNAL)) == -1)
      {
          perror("client send");
          break;
      }
      numBytes += recvBytes;
      bytesleft -= recvBytes;
  }

  return recvBytes == -1 ? false : true;
}