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
#include <sched.h>

#include "../../client.hpp"
#include "../../pugixml/pugiconfig.hpp"
#include "../../pugixml/pugixml.hpp"
#include "genXml.hpp"

/*   the host name and port number, for debugging only   */
/*   may change if server executing in another machine   */
#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT "12345"

// change MAX_THREAD to increase or decrease the number of queries sent
#define MAX_THREAD 500
#define BUFF_SIZE 10240
int x = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
using std::string;

std::string produceXML(int id){
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
  return ans;
}

// set up the client and start sending message to server
void * handler(void * arg) {
  std::string port_raw = "12345";
  size_t port_num = std::stoul(port_raw);
  Client client = Client(port_num, "vcm-30576.vm.duke.edu");
  std::string ans = *(std::string*)arg;
  client.sendRequest(ans.c_str(), ans.size());
  try
  {
    std::string response = client.recvResponse();
  }
  catch(const std::exception& e)
  {
    return NULL;
  }
  return NULL;
}

int main(int argc, char ** argv) {
  int cpu_num = sched_getcpu();
  std::cout << "Current processor number: " << cpu_num << std::endl;
  int threads[MAX_THREAD];
  pthread_attr_t thread_attr[MAX_THREAD];
  pthread_t thread_ids[MAX_THREAD];
  std::vector<string> requests(MAX_THREAD);
  int x = rand() % MAX_THREAD;
  genXml gg;
  for (int i = 0; i < x; i ++ ){
    requests[i] = gg.generateRandomCreateXml();
  }
  for (int i = x; i < MAX_THREAD; i ++ ){
    requests[i] = gg.generateRandomTransXml();
  }
  // for (int i = 0; i < MAX_THREAD; ++i){
  //   requests[i] = produceXML(i);
  // }

  auto t1 = std::chrono::steady_clock::now();
  for (int i = 0; i < MAX_THREAD; ++i) {
    threads[i] = pthread_create(&thread_ids[i], NULL, handler, &requests[i]);
  }
  for (int i = 0; i < MAX_THREAD; ++i) {
    pthread_join(thread_ids[i], NULL);
  }

  auto t2 = std::chrono::steady_clock::now();
  double dr_s = std::chrono::duration<double>(t2 - t1).count();
  std::cout << "run time: " << dr_s << " s" << std::endl;
  return 0;
}
