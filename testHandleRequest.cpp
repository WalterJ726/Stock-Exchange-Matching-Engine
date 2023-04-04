#include "Server.hpp"
#include "client.hpp"

int main(int argc, char** argv)
{
    // browser -> proxy server: test httprequest & recv string
    // proxy server -> google.com: response(string)
    Client client = Client(12345, "vcm-30576.vm.duke.edu");

    std::string filename = argv[1];
    std::ifstream xml_file(filename);
    std::string line, msg;
    if (!xml_file){
        std::cout << "can not open file" << std::endl;
    } else {
        while (getline(xml_file, line)){
            line += std::string("\n");
            msg += line;
        }
    }
    std::cout << msg << std::endl;
    client.sendRequest(msg.c_str(), msg.size());


    std::string response = client.recvResponse();
    std::cout << response << std::endl;
    return 0;
}