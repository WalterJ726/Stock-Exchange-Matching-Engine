#include "Server.hpp"
#include "client.hpp"

int main()
{
    // browser -> proxy server: test httprequest & recv string
    // proxy server -> google.com: response(string)
    Client client = Client(12345, "vcm-30576.vm.duke.edu");

    // const char* msg = "173"
    //         "<?xml version="1.0" encoding="UTF-8"?>"
    //         "<create>"
    //         "   <account id="123456" balance="1000"/>"
    //         "   <symbol sym="SPY">"
    //         "       <account id="123456">100000</account>"
    //         "   </symbol>"
    //         "</create>";
    
    std::string filename = "test.xml";
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
    return 0;
}