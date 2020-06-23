#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>
#include "server.hpp"
#include <cstring>

#include "CLI11.hpp"

using namespace std;
int main(int argc,char** argv){

    CLI::App app("tool for hole punching");
    app.require_subcommand();

    //client 模式
    auto bind = app.add_subcommand("bind","bind to the remote system");
    // bind->require_subcommand();
    std::string remoteServer,remotePort,localPort;
    bind->add_option("-s,--server",remoteServer,"remote server on public network")->mandatory();
    bind->add_option("-P,--remotePort",remotePort,"the port of remote server")->mandatory();
    bind->add_option("-p,--localPort",localPort,"the port of local program")->mandatory();
    bind->callback([&](){
        //发送请求到远端服务器，返回收到当前mapping之后的ip port
         auto intRemotePort = boost::lexical_cast<unsigned short>(remotePort);
         auto intLocalPort = boost::lexical_cast<unsigned short>(localPort);
         io_service bindIoServer;
         client clienta(bindIoServer,intLocalPort);
        clienta.sendTo(remoteServer,intRemotePort,localPort);
    });
    
    //服务端模式 本地默认为0.0.0.0 
    std::string remoteServerIp,remoteServerPort,localServerPort;
    auto servermode = app.add_subcommand("servermode","a udp server for listening");
    servermode->add_option("-s,--remoteserverip",remoteServerIp,"remote server on public network")->mandatory();
    servermode->add_option("-P,--remoteport",remoteServerPort,"the port of remote server")->mandatory();
    servermode->add_option("-p,--localport",localServerPort,"the port of remote server")->mandatory();
 
    
    servermode->callback([&](){
        //1.向远程发送bind请求
        //2.启动udp监听,主动请求。
        try
        {
            boost::asio::io_service io_service;

            auto intRemotePort = boost::lexical_cast<unsigned short>(remoteServerPort);
            auto intLocalPort = boost::lexical_cast<unsigned short>(localServerPort);
            // auto intPort = boost::lexical_cast<int>(localServerPort);
            // client sclient(io_service,intLocalPort);
            // sclient.sendTo(remoteServerIp,intRemotePort,localServerPort);

            udpServer server(io_service,intLocalPort);
            server.clientHandler->sendTo(remoteServerIp,intRemotePort,localServerPort);
            //阻塞在这里
            io_service.run();
            return 0;
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
            return -1;
        }
    });

    CLI11_PARSE(app, argc, argv);
    return 0;



    
}

/*
note : 增加一个server 用于探测客户端所在的nat设备的类型：
nat type:
1. Full cone
2. Restricted cone
3. Port restricted cone 

功能有：
1. 探测
2. 管理注册上来的client的ip port 以及nat 类型
*/