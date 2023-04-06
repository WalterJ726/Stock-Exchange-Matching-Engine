#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "Server.hpp"
#include "client.hpp"
#include "pugixml/pugiconfig.hpp"
#include "pugixml/pugixml.hpp"

/*   the host name and port number, for debugging only   */
/*   may change if server executing in another machine   */
#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT "12345"

// change MAX_THREAD to increase or decrease the number of queries sent
#define MAX_THREAD 1000
#define BUFF_SIZE 10240
int x = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// set up the client and start sending message to server
void * handler(void * arg) {
  std::string port_raw = "12345";
  size_t port_num = std::stoul(port_raw);

  Client client = Client(port_num, "127.0.0.1");
  std::stringstream ss;

  pugi::xml_document request;
  pugi::xml_node decl = request.append_child(pugi::node_declaration);
  decl.append_attribute("version") = "1.0";
  decl.append_attribute("encoding") = "UTF-8";
  pugi::xml_node result_node = request.append_child("create");
  pugi::xml_node account_child = result_node.append_child("account");
  account_child.append_attribute("balance") = "50000";
  pugi::xml_node symbol_child = result_node.append_child("symbol");
  symbol_child.append_attribute("sym") = "SPY";
  pugi::xml_node symbol_child_child = symbol_child.append_child("account");
  pthread_mutex_unlock(&mutex);
  account_child.append_attribute("id") = x;
  symbol_child_child.append_attribute("id") = x++;
  // std::cout << x << std::endl;
  pthread_mutex_unlock(&mutex);
  symbol_child_child.append_child(pugi::node_pcdata).set_value("200");

  // std::string msg = ss.str();
  std::ostringstream oss;
  request.save(oss);
  std::string rrequest = oss.str();
  std::string rrquest_size = std::to_string(rrequest.size());

  std::string ans = rrquest_size + "\n" + rrequest;
  client.sendRequest(ans.c_str(), ans.size());

  std::string response = client.recvResponse();
  return NULL;
}

int main(int argc, char ** argv) {
  int threads[MAX_THREAD];
  pthread_attr_t thread_attr[MAX_THREAD];
  pthread_t thread_ids[MAX_THREAD];
  char * filename = argv[1];
  auto t1 = std::chrono::steady_clock::now();
  for (int i = 0; i < MAX_THREAD; ++i) {
    threads[i] = pthread_create(&thread_ids[i], NULL, handler, filename);
    usleep(1000);
  }
  for (int i = 0; i < MAX_THREAD; ++i) {
    pthread_join(thread_ids[i], NULL);
  }

  auto t2 = std::chrono::steady_clock::now();
  double dr_s = std::chrono::duration<double>(t2 - t1).count();
  std::cout << "run time: " << dr_s << " s" << std::endl;
  return 0;
}
