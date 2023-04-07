#include "Server.hpp"


int main()
{
    // if(daemon(1, 0) == -1){
    //     std::cout<<"error\n"<<std::endl;
    //     exit(-1);
    // }
    Server* server_daemon = nullptr;
    try
    {
        server_daemon = new Server(12345);
        if (server_daemon->getErrorSign()){
            std::cout << "fail to initialize server" << std::endl;
        }
        server_daemon->startRun();

    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        std::cout << "fail to initialize server" << std::endl;
    }
    delete server_daemon;
    return EXIT_SUCCESS;
}
