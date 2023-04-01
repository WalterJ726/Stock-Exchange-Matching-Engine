#include "cptl.h"
#include "pugiconfig.hpp"
#include "pugixml.hpp"

class Server{
private:
    const int port_num;

public:
    explicit Server(const int port_num) : port_num(port_num) {};
    void startRun();

}