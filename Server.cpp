#include "Server.hpp"

void Server::startRun() {
  std::cout << "start Run server" << std::endl;
  std::threadpool executor{50};
  Database db("exchange", "postgres", "passw0rd");
  db.connect();
  db.initialize();
  db.disconnect();
  // start getting order from client
  while (true) {
    int client_connection_fd = tryAccept();
    if (client_connection_fd == -1) {
      std::cout << "accpet failed" << std::endl;
      continue;
    }
    executor.commit(handleRequest, client_connection_fd).get();
  }
}

void Server::handleRequest(const int & client_connection_fd) {
  Database db("exchange", "postgres", "passw0rd");
  db.connect();
  std::vector<char> buff(MAX_TCP_PACKET_SIZE);
  // recv data
  size_t numbytes = recv(client_connection_fd, &buff.data()[0], MAX_TCP_PACKET_SIZE, 0);
  // get length of xml
  std::string xml_len;
  for (size_t i = 0; i < buff.size(); i++) {
    if (buff[i] == '\n') {
      break;
    }
    else {
      xml_len += buff[i];
    }
  }

  size_t xml_tot;
  try {
    xml_tot = std::stoul(xml_len, nullptr, 0);
  }
  catch (const std::exception & e) {
    std::cerr << e.what() << '\n';
    std::cout << "fail to convert number" << std::endl;
  }

  // continue to recv data
  size_t cur_bytes = numbytes - xml_len.size() - 1;
  size_t len = 0;
  while (cur_bytes < xml_tot) {
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
  if (!doc.load_string(xml_str.c_str())) {
    std::cerr << "Failed to load XML string." << std::endl;
  }

  // Access the root element of the XML document
  pugi::xml_node root = doc.document_element();
  std::string root_name = root.name();

  pugi::xml_document response_raw;
  if (root_name == "create") {
    std::cout << "start to process create" << std::endl;
    response_raw = process_create(doc, client_connection_fd, db);
  }
  else if (root_name == "transactions") {
    std::cout << "start to process transactions" << std::endl;
    response_raw = process_transactions(doc, client_connection_fd, db);
  }
  else {
    std::cout << "wrong xml" << std::endl;
  }
  // send reponse back
  response_raw.print(std::cout);
  std::ostringstream oss;
  response_raw.save(oss);
  std::string response = oss.str();
  sendAllData(client_connection_fd, response.c_str(), response.size());
  close(client_connection_fd);
  db.disconnect();
}

void set_invalid_create_symbol_child(string & account_id_raw, string & sym, pugi::xml_node & status_node, const char* msg) {
    pugi::xml_node invalid_symbol_child = status_node.append_child("error");
    invalid_symbol_child.append_attribute("sym") = sym.c_str();
    invalid_symbol_child.append_attribute("id") = account_id_raw.c_str();
    invalid_symbol_child.append_child(pugi::node_pcdata)
        .set_value(msg);
}

void set_invalid_create_account_child(string & account_id_raw, pugi::xml_node & status_node, const char* msg) {
    pugi::xml_node invalid_account_child = status_node.append_child("error");
    invalid_account_child.append_attribute("id") = account_id_raw.c_str();
    invalid_account_child.append_child(pugi::node_pcdata)
        .set_value(msg);
}

pugi::xml_document Server::process_create(const pugi::xml_document & doc,
                                          const int & client_connection_fd,
                                          Database db) {
  pugi::xml_document response;
  pugi::xml_node decl = response.append_child(pugi::node_declaration);
  decl.append_attribute("version") = "1.0";
  decl.append_attribute("encoding") = "UTF-8";
  pugi::xml_node result_node = response.append_child("results");
  pugi::xml_node root = doc.document_element();
  for (pugi::xml_node cur = root.first_child(); cur; cur = cur.next_sibling()) {
    if (std::string(cur.name()) == "account") {
      // TODO: id and balance is not valid and add error msg
      bool hasError = false;
      std::cout << "start to process creating account" << std::endl;
      std::string account_id = cur.attribute("id").value();
      std::string balance_raw = cur.attribute("balance").value();
      if (account_id == "" || balance_raw == ""){
        set_invalid_create_account_child(account_id, result_node, "cannot get id or balance");
        continue;
      }
      size_t balance;
      try
      {
        balance = std::stoul(balance_raw);
      }
      catch(const std::exception& e)
      {
        std::cerr << e.what() << '\n';
        hasError = true;
      }
      if (!hasError && db.insert_account(account_id, balance)) {
        pugi::xml_node created_account_child = result_node.append_child("created");
        created_account_child.append_attribute("id") = account_id.c_str();
      }
      else {
        set_invalid_create_account_child(account_id, result_node, "Account already exists");
      }
    }
    else if (std::string(cur.name()) == "symbol") {
      // TODO: sym is not valid and add error msg
      // TODO: the indent of error msg
      std::cout << "start to process creating symbol" << std::endl;
      std::string sym = cur.attribute("sym").value();
      bool hasError = false;
      pugi::xml_node invalid_symbol_child;
      if (sym == ""){
        hasError = true;
        invalid_symbol_child = result_node.append_child("error");
        invalid_symbol_child.append_attribute("sym") = sym.c_str();
        invalid_symbol_child.append_child(pugi::node_pcdata)
            .set_value("symbol invalid");
      }
      for (pugi::xml_node sym_create = cur.first_child(); sym_create;
           sym_create = sym_create.next_sibling()) {
        std::string account_id = sym_create.attribute("id").value();
        if (account_id == ""){
          hasError = true;
        }
        std::string num_raw = sym_create.child_value();
        size_t num;
        try
        {
          num = std::stoul(num_raw);
        }
        catch(const std::exception& e)
        {
          std::cerr << e.what() << '\n';
          hasError = true;
        }
        if (!hasError && db.insert_sym(account_id, sym, num)) {
          pugi::xml_node created_account_child = result_node.append_child("created");
          created_account_child.append_attribute("sym") = sym.c_str();
          created_account_child.append_attribute("id") = account_id.c_str();
        }
        else {
          // TODO: Will this function produce error?
          set_invalid_create_symbol_child(account_id,sym,invalid_symbol_child,"symbol create error");
          std::cout << "Will this function produce error" << std::endl;
        }
      }
    }
    else {
      pugi::xml_node error_child = result_node.append_child("error");
      error_child.append_child(pugi::node_pcdata).set_value("invalid child name");
      std::cout << "bad child in create" << std::endl;
      // throw std::exception();
    }
  }
  return response;
  // TODO: send response back
}

void set_invalid_trans_id_child(string & trans_id_raw, pugi::xml_node & status_node, const char* msg) {
  pugi::xml_node invalid_trans_id_child = status_node.append_child("error");
  invalid_trans_id_child.append_attribute("id") = trans_id_raw.c_str();
  invalid_trans_id_child.append_child(pugi::node_pcdata)
      .set_value("Given trans_id does not belong to the current account");
}

//set cancelld_child
void set_cancelled_child(const int trans_id, Database db, pugi::xml_node & status_node) {
  set<pair<int, int> > cancelled_set;
  db.set_cancelled_result(trans_id, cancelled_set);
  if (cancelled_set.size() != 0) {
    for (set<pair<int, int> >::const_iterator it = cancelled_set.begin();
         it != cancelled_set.end();
         ++it) {
      pugi::xml_node cancelled_child = status_node.append_child("canceled");
      cancelled_child.append_attribute("shares") = std::to_string(it->first).c_str();
      cancelled_child.append_attribute("times") = std::to_string(it->second).c_str();
    }
  }
}

//set executed_child
void set_executed_child(const int trans_id, Database db, pugi::xml_node & status_node) {
  set<pair<int, pair<double, int> > > executed_set;
  db.set_executed_result(trans_id, executed_set);
  if (executed_set.size() != 0) {
    for (set<pair<int, pair<double, int> > >::const_iterator it = executed_set.begin();
         it != executed_set.end();
         ++it) {
      pugi::xml_node executed_child = status_node.append_child("executed");
      executed_child.append_attribute("shares") = std::to_string(it->first).c_str();
      executed_child.append_attribute("price") = std::to_string(it->second.first).c_str();
      executed_child.append_attribute("times") =
          std::to_string(it->second.second).c_str();
    }
  }
}


void set_invalid_order_child(string & amount_raw, string& sym, string& limit_raw, pugi::xml_node & status_node, const char* msg) {
    pugi::xml_node invalid_insert_child = status_node.append_child("error");
    invalid_insert_child.append_attribute("sym") = sym.c_str();
    invalid_insert_child.append_attribute("amount") = amount_raw.c_str();
    invalid_insert_child.append_attribute("limit") = limit_raw.c_str();
    invalid_insert_child.append_child(pugi::node_pcdata)
        .set_value(msg);
}

pugi::xml_document Server::process_transactions(const pugi::xml_document & doc,
                                                const int & client_connection_fd,
                                                Database db) {
  pugi::xml_document response;
  pugi::xml_node decl = response.append_child(pugi::node_declaration);
  decl.append_attribute("version") = "1.0";
  decl.append_attribute("encoding") = "UTF-8";
  pugi::xml_node result_node = response.append_child("results");
  pugi::xml_node root = doc.document_element();
  std::string account_id = root.attribute("id").value();
  bool invalidAccount = !db.find_account(account_id);
  for (pugi::xml_node cur = root.first_child(); cur; cur = cur.next_sibling()) {
      if (invalidAccount){
        pugi::xml_node invalid_account = result_node.append_child("error");
        invalid_account.append_attribute("id") = account_id.c_str();
        invalid_account.append_child(pugi::node_pcdata).set_value("Account ID doesn't exist");
        continue;
      }
      if (std::string(cur.name()) == "query") {
        std::cout << "start to query" << std::endl;
        bool hasError = false;
        std::string trans_id_raw = cur.attribute("id").value();
        if (trans_id_raw == ""){
          hasError = true;
        }
        int trans_id;
        int account_id_int;
        try
        {
            trans_id = std::stoi(trans_id_raw);
            account_id_int = std::stoi(account_id);
        }
        catch(const std::exception& e)
        {
          std::cerr << e.what() << '\n';
          hasError = true;
        }
        pugi::xml_node status_node = result_node.append_child("status");
        status_node.append_attribute("id") = trans_id_raw.c_str();
        if (hasError){
          set_invalid_trans_id_child(trans_id_raw, status_node, "query parse failed");
          continue;
        } else if (!db.is_own_trans(trans_id, account_id_int)) {
          set_invalid_trans_id_child(trans_id_raw, status_node, "Given trans_id does not belong to the current account");
          continue;
        }
        else {
          // for open
          int open_shares = 0;
          db.set_open_result(trans_id, open_shares);
          if (open_shares != 0) {
            pugi::xml_node open_child = status_node.append_child("open");
            open_child.append_attribute("shares") = std::to_string(open_shares).c_str();
          }
          // for cancelled
          set_cancelled_child(trans_id, db, status_node);
          // for executed
          set_executed_child(trans_id, db, status_node);
        }
      }
      else if (std::string(cur.name()) == "cancel") {
        std::cout << "start to cancel" << std::endl;
        bool hasError = false;
        std::string trans_id_raw = cur.attribute("id").value();
        if (trans_id_raw == ""){
          hasError = true;
        }
        int trans_id;
        int account_id_int;
        try
        {
            trans_id = std::stoi(trans_id_raw);
            account_id_int = std::stoi(account_id);
        }
        catch(const std::exception& e)
        {
          std::cerr << e.what() << '\n';
          hasError = true;
        }
        pugi::xml_node canceled_node = result_node.append_child("canceled");
        canceled_node.append_attribute("id") = trans_id_raw.c_str();

        if (hasError){
          set_invalid_trans_id_child(trans_id_raw, canceled_node, "cancel failed parse");
          continue;
        } else if (!db.is_own_trans(trans_id, account_id_int)) {
          set_invalid_trans_id_child(trans_id_raw, canceled_node, "Given trans_id does not belong to the current account");
          continue;
        }
        else {
          db.cancel_transaction(trans_id);
          // for cancelled
          set_cancelled_child(trans_id, db, canceled_node);
          // for executed
          set_executed_child(trans_id, db, canceled_node);
        }
      }
      else if (std::string(cur.name()) == "order") {
        std::cout << "start to order" << std::endl;
        bool hasError = false;
        // TODO: sym amount limit invalid (does not exist, name is wrong)
        std::string sym = cur.attribute("sym").value();
        std::string amount_raw = cur.attribute("amount").value();
        std::string limit_raw = cur.attribute("limit").value();
        if (sym == "" || amount_raw == "" || limit_raw == ""){
          hasError = true;
        }
        int amount;
        double limit;
        try
        {
          amount = std::stoi(amount_raw);
          limit = std::stod(limit_raw);
        }
        catch(const std::exception& e)
        {
          std::cerr << e.what() << '\n';
          hasError = true;
        }
        size_t trans_id;
        if (hasError){
          set_invalid_order_child(account_id, sym, limit_raw, result_node, "open order failed parse");
          continue;
        } else if (db.insert_order(account_id, sym, amount, limit, trans_id)) {
          pugi::xml_node open_order_child = result_node.append_child("opened");
          open_order_child.append_attribute("sym") = sym.c_str();
          open_order_child.append_attribute("amount") = amount_raw.c_str();
          open_order_child.append_attribute("limit") = limit_raw.c_str();
          open_order_child.append_attribute("id") = std::to_string(trans_id).c_str();
          // execute an order
          db.executed_order(account_id, sym, amount, limit, trans_id);
        } else {
          set_invalid_order_child(account_id, sym, limit_raw, result_node, "open order failed creating");
        }
      }
      else {
        pugi::xml_node error_child = result_node.append_child("error");
        error_child.append_child(pugi::node_pcdata).set_value("invalid child name");
        std::cout << "bad child in transactions" << std::endl;
      }
}
return response;
                                                }

Server::Server(int port_num) : port_num(port_num) {
  hasError = 0;
  if (setUpStruct() == -1) {
    hasError = 1;
  }
  if (initSocketFd() == -1) {
    hasError = 1;
  }
  if (tryBind() == -1) {
    hasError = 1;
  }
  if (startListen() == -1) {
    hasError = 1;
  }
}

int Server::setUpStruct() {
  int status;
  memset(&host_info, 0, sizeof(host_info));

  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;
  std::string port_str = std::to_string(port_num);
  status = getaddrinfo(nullptr, port_str.c_str(), &host_info, &host_info_list);
  if (status != 0) {
    // "Error: cannot get address info for host"
    return -1;
  }
  return status;
}

int Server::initSocketFd() {
  socket_fd = socket(host_info_list->ai_family,
                     host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    // "Error: cannot create socket" << endl;
    return -1;
  }
  return socket_fd;
}

int Server::tryBind() {
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

int Server::startListen() {
  int status;
  status = listen(socket_fd, 100);
  if (status == -1) {
    // "Error: cannot listen on socket"
    return -1;
  }
  freeaddrinfo(host_info_list);
  return status;
}

int Server::tryAccept() {
  struct sockaddr_storage socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);
  client_connection_fd =
      accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
  if (client_connection_fd == -1) {
    // "Error: cannot accept connection on socket"
    return -1;
  }
  return client_connection_fd;
}

int Server::getErrorSign() {
  return hasError;
}

std::string Server::recvData(int flag) {
  char recvbuff[MAX_TCP_PACKET_SIZE];
  int numbytes;
  if ((numbytes = recv(client_connection_fd, recvbuff, MAX_TCP_PACKET_SIZE, flag)) ==
      -1) {
    hasError = 1;
    return nullptr;
  }
  // recvbuff[numbytes] = '\0';
  return std::string(recvbuff, numbytes);
}

bool Server::sendAllData(int sockfd, const char * msg, size_t size) {
  size_t numBytes = 0;
  size_t bytesleft = size;
  int recvBytes = 0;
  while ((numBytes < bytesleft)) {
    if ((recvBytes = send(sockfd, msg + numBytes, size, MSG_NOSIGNAL)) == -1) {
      perror("client send");
      break;
    }
    numBytes += recvBytes;
    bytesleft -= recvBytes;
  }

  return recvBytes == -1 ? false : true;
}
