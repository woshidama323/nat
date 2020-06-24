#include <boost/asio.hpp>
#include <iostream>
#include "client.hpp"

client::client(io_service &bindIoServer,unsigned short intPort,std::shared_ptr<ip::udp::socket> _clientSocketPtr)
{
    _clientSocket = _clientSocketPtr;
}

std::string client::LocalIp(std::string & ip){
    try {
        boost::asio::io_service netService;
        udp::resolver   resolver(netService);
        udp::resolver::query query(udp::v4(), "www.baidu.com", "");
        udp::resolver::iterator endpoints = resolver.resolve(query);
        udp::endpoint ep = *endpoints;
        udp::socket socket(netService);
        socket.connect(ep);
        boost::asio::ip::address addr = socket.local_endpoint().address();
        // std::cout << "My IP according to google is: " << addr.to_string() << std::endl;
        return addr.to_string();
    } catch (std::exception& e){
        std::cerr << "Could not deal with socket. Exception: " << e.what() << std::endl;
        return std::string{};
    }
    // return ip;
}

int client::sendTo(std::string &remoteIp,unsigned short &remotePort,std::string & msg){//,std::string &localPort
    try{
        // io_service bindIoServer;
        // udpServer* udpServer = new udpServer;
        // auto hello =  new udpServer();
        // hello->getLocalIP();
        std::string iptmp = "122.29.3.11";
        auto localIp = LocalIp(iptmp);
        // // std::cout<<hello->_localIp
        //获取本地ip地址，当前服务的port号
        // std::string msg = std::string("{\"msgtype\":\"detect\",\"data\":{\"localip\":\"why\"}}");
        // json msgj2 = {
        //     {"msgtype","detect"},
        //     {"data",{
        //         {"localip",localIp},
        //         {"localport",localPort}
        //     }}
        // };
        // auto msg = msgj2.dump();
        // auto intPort = boost::lexical_cast<unsigned short>(localPort);
        // auto intRemotePort = boost::lexical_cast<unsigned short>(remotePort);
        // if(std::numeric_limits<unsigned short>::max())
        
        auto remoteEndPoint = boost::asio::ip::udp::endpoint(ip::address().from_string(remoteIp),remotePort);
        boost::system::error_code ignored_error;
        //why need open
        // bindSocket.open(boost::asio::ip::udp::v4());
        std::cout<<"starting respose:"<<remoteIp<<" port:"<<remotePort<<std::endl;
        _clientSocket->send_to(buffer(msg),remoteEndPoint,0,ignored_error);
        std::cout<<"finished the sending"<<std::endl;
        return 0;
    }
    catch(const std::exception& e)
    {
        std::cerr << "Got errors: " <<e.what() << '\n';
        return -1;
    }    

}