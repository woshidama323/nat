#include "server.hpp"
#include <boost/lexical_cast.hpp>
#include <vector>
using namespace boost::asio;
using namespace boost::asio::ip;

udpServer::udpServer(io_service& io_service,const unsigned short port)
:_socketPtr{std::make_shared<ip::udp::socket>(io_service,ip::udp::endpoint(ip::udp::v4(),port))},clientHandler{make_shared<client>(io_service,port,_socketPtr)}
{
    // clientHandler->_clientSocket = _socket;
    //初始化managenode;
    manHost = make_shared<managerHost>();
    // getLocalIP();
    // _socket.bind(udp::endpoint(ip::address().from_string(_localIp),61000));
    std::cout<< _socketPtr->local_endpoint().address().to_string()<<" port: "<<_socketPtr->local_endpoint().port()<<std::endl;
    //放在这里，先发送消息到一个服务器中

    startReceive();
}

void udpServer::startReceive(){
    _socketPtr->async_receive_from(
        boost::asio::buffer(_recvBuffer),
        _remoteEndpoint,
        boost::bind(&udpServer::handleReceive,this,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred)
    );
}

void udpServer::handleReceive(const boost::system::error_code &error,std::size_t size){
    if (!error || error == boost::asio::error::message_size){
        //假设已经收到了string的数据，
        //收到有效消息之后，获取到对端的ip 和 port 
        auto seeRemoteIp = _remoteEndpoint.address().to_string();
        auto seeRemotePort = _remoteEndpoint.port();

        std::cout << seeRemoteIp <<" "<< seeRemotePort<<std::endl;

        try
        {
            //检查是一个有效的json字符串
            if(json::accept(_recvBuffer)){
                auto recvJson = json::parse(_recvBuffer);
                if(recvJson.find("msgtype") != recvJson.end()){
                    //说明有这个字段，
                    std::string msgType{""};
                    recvJson["msgtype"].get_to(msgType);
                    if (msgType == "detect"){

                        auto localIpStr = recvJson["data"]["localip"].get<std::string>();
                        auto localPortStr = recvJson["data"]["localport"].get<std::string>();
                        auto localPortInt = boost::lexical_cast<unsigned short>(localPortStr);

                        std::cout<<"detect comming:"<< recvJson.dump() <<std::endl;
                        if((seeRemoteIp != localIpStr) || (seeRemotePort != localPortInt) ){
                            std::cout<< "send mapping tyep" << std::endl;
                            //装入list中
                            struct hostInfo nodeInfo1, nodeInfo2;

                            nodeInfo1.ip = seeRemoteIp;
                            nodeInfo1.port = seeRemotePort;

                            nodeInfo2.ip = recvJson["data"]["localip"];
                            nodeInfo2.port = localPortInt;

                            std::vector<struct hostInfo> hostInfos;
                            hostInfos.push_back(nodeInfo1);
                            hostInfos.push_back(nodeInfo2);

                            manHost->addNewNode(hostInfos);
                            // manHost.
                            clientHandler->sendTo(seeRemoteIp,seeRemotePort,localPortStr);
                            //放入到自己的队列中，然后返回给地方其他人的节点信息，
                        }else{
                            //ip 和 port 都相同
                            std::cout << "====== there all the same =====" << std::endl;
                            std::cout << "remote ip: " <<seeRemoteIp <<" remote port: "<< seeRemotePort<<std::endl;
                            std::cout << "local ip: " <<localIpStr<<" local port: "<< localPortInt <<std::endl;

                        }

                    }else if (msgType == "update"){
                        std::cout<<"update comming"<<std::endl;
                    }else if (msgType == "holdpunching"){
                        std::cout<<"holdpunching comming"<<std::endl;
                    }else{
                        std::cout<<"not support"<<std::endl;
                    }
                }
            }
        }
        catch(const std::exception& e)
        {
            // output exception information
            std::cout << "message: " << e.what() << '\n';
                    // << "exception id: " << e.id << '\n'
                    // << "byte position of error: " << e.byte << std::endl;
        }

        //如果没有连接，这里就不再继续，
        
        startReceive();
        //这里需要过滤请求类型
    }
}

void udpServer::sendRequest(std::string msg, const std::string & desIp,const unsigned short desPort){

    io_service io_service;
    ip::udp::socket socket(io_service);
    //组装目标endpoint 

    //第一步获取本地节点ip 固定一个端口
    auto loopIp = ip::udp::endpoint(ip::address::from_string("127.0.0.1"),9);
    try
    {
        boost::system::error_code ignored_error;
        auto msg = std::string("test");
        socket.connect(loopIp);
        std::cout<< socket.local_endpoint().address().to_string() <<std::endl;
        socket.send_to(boost::asio::buffer(msg),loopIp,0,ignored_error);

        // socket.send_to(boost::asio::buffer(msg),loopIp,0,ignored_error);
    
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    // auto remote = ip::udp::endpoint(ip::address::from_string(desIp),desPort);

    
}

void udpServer::getLocalIP(){
    // io_service io_service;
    // ip::udp::socket socket(io_service);
    // //第一步获取本地节点ip 固定一个端口
    // auto loopIp = ip::udp::endpoint(ip::address::from_string("127.0.0.1"),9);
    // try
    // {
    //     auto msg = std::string("test");
    //     socket.connect(loopIp);
    //     _localIp = socket.local_endpoint().address().to_string();
    //     std::cout<< _localIp <<std::endl;
        
    // }
    // catch(const std::exception& e)
    // {
    //     std::cerr << e.what() << '\n';
    // }
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
        _localIp = addr.to_string();
    } catch (std::exception& e){
        std::cerr << "Could not deal with socket. Exception: " << e.what() << std::endl;
    }
}