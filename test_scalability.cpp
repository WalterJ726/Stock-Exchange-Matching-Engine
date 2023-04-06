#include <arpa/inet.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <time.h>
#include <unistd.h>
#include "pugixml/pugiconfig.hpp"
#include "pugixml/pugixml.hpp"
#include "Server.hpp"
#include "client.hpp"

/*   the host name and port number, for debugging only   */
/*   may change if server executing in another machine   */
#define SERVER_ADDR "vcm-30576.vm.duke.edu"
#define SERVER_PORT "12345"

// change MAX_THREAD to increase or decrease the number of queries sent
#define MAX_THREAD 100
#define BUFF_SIZE 10240
int x = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// set up the client and start sending message to server
void *handler(void *arg) {
  std::string port_raw = "12345";
  size_t port_num = std::stoul(port_raw);

  Client client = Client(port_num, "vcm-30576.vm.duke.edu");
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
  client.sendRequest(rrequest.c_str(), rrequest.size());

  std::string response = client.recvResponse();
}

int main(int argc, char **argv) {
  int threads[MAX_THREAD];
  pthread_attr_t thread_attr[MAX_THREAD];
  pthread_t thread_ids[MAX_THREAD];
  char* filename = argv[1];
  for (int i = 0; i < MAX_THREAD; ++i) {
    threads[i] = pthread_create(&thread_ids[i], NULL, handler, filename);
    usleep(1000);
  }
  for (int i = 0; i < MAX_THREAD; ++i) {
    pthread_join(thread_ids[i], NULL);
  }
  return 0;
}
