#include <pthread.h>

#include "Server.hpp"
#include "client.hpp"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void * handleRequest(void * arg) {
  std::string port_raw = "12345";
  size_t port_num = std::stoul(port_raw);

  Client client = Client(port_num, "127.0.0.1");

  int id = rand() % 1000 + 1;
  pthread_mutex_lock(&mutex);
  std::cout << "current account id : " << id << std::endl;
  pthread_mutex_unlock(&mutex);
  std::stringstream ss;
  ss << "173\n"
     << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
     << "<create>\n"
     << "<account id=\"" << id << "\" balance=\"50000\"/>\n"
     << "<symbol sym=\"SPY\">"
     << "<account id=\"" << id << "\">200</account>\n"
     << "</symbol>\n"
     << "</create>\n";

  string msg = ss.str();

  client.sendRequest(msg.c_str(), msg.size());

  std::string response = client.recvResponse();
  return NULL;
}

int main(int argc, char ** argv) {
  srand((unsigned int)time(NULL));

  for (int i = 0; i < 1000; ++i) {
    pthread_t thread;
    int ret = pthread_create(&thread, NULL, handleRequest, NULL);
    std::cout << "current i: " << i << std::endl;
    if (ret != 0) {
      std::cout << "pthread create error" << std::endl;
    }
  }

  sleep(2);
  return 0;
}
