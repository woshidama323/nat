#include <boost/asio.hpp>
#include <iostream>
#include "json.hpp"
#include <boost/lexical_cast.hpp>


using namespace boost::asio::ip;
using namespace boost::asio;
using json = nlohmann::json;

class client{
public:
    client(io_service &bindIoServer,unsigned short intPort,std::shared_ptr<ip::udp::socket> _clientSocket);
    ~client(){};

    std::shared_ptr<ip::udp::socket> _clientSocket;
    // ip::udp::socket _clientSocket;
    std::string LocalIp(std::string & ip);
    int sendTo(std::string &remoteIp,unsigned short &remotePort,const std::string & msg);
};
