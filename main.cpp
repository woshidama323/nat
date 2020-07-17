#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>
#include "server.hpp"
#include <cstring>

#include "CLI11.hpp"
#include "Node.hpp"

using namespace std;
#define MY_TRACE(msg, ...) \
    MyTrace(__LINE__, __FILE__, msg, __VA_ARGS__)

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
         auto socketPtr = std::make_shared<ip::udp::socket>(bindIoServer,ip::udp::endpoint(ip::address().from_string("0.0.0.0"),intLocalPort));
         client clienta(bindIoServer,intLocalPort,socketPtr);
        json msgj2 = {
            {"msgtype","detect"},
            {"data",{
                {"localip","10.0.0.1"},
                {"localport",localPort}
            }}
        };
        auto str = msgj2.dump();
        clienta.sendTo(remoteServer,intRemotePort,str);
    });
    
    //服务端模式 本地默认为0.0.0.0 
    std::string remoteServerIp,remoteServerPort,localServerPort,localIp;
    bool mainNode;
    auto servermode = app.add_subcommand("servermode","a udp server for listening");
    servermode->add_option("-s,--remoteserverip",remoteServerIp,"remote server on public network")->mandatory();
    servermode->add_option("-P,--remoteport",remoteServerPort,"the port of remote server")->mandatory();
    servermode->add_option("-p,--localport",localServerPort,"the port of remote server")->mandatory();
    servermode->add_option("-i,--ip",localIp,"the port of remote server")->mandatory();
    servermode->add_flag("-m,--mainnode",mainNode,"set as a public node");
 
    
    servermode->callback([&](){
        
        /*
            1.实例化node类
                1.1 设置本地node id
                1.2 启动udpserver （两个）
                1.3 启动dispatchqueue
            2.
        */
        
            
        //2.启动udp监听,主动请求。
        try
        {
            boost::asio::io_service io_service;

            auto intRemotePort = boost::lexical_cast<unsigned short>(remoteServerPort);
            auto intLocalPort = boost::lexical_cast<unsigned short>(localServerPort);
            // auto intPort = boost::lexical_cast<int>(localServerPort);
            // client sclient(io_service,intLocalPort);
            // sclient.sendTo(remoteServerIp,intRemotePort,localServerPort);

            Node node(io_service,remoteServerIp,intRemotePort,intLocalPort,mainNode,localIp);

            //发送探测消息,然后进入监控模型，等待回复，如果有回复，则消息体中会有路由表信息，不论你在不在nat下，都会发
            node.startDetect();
            //获取本地地址
            // auto getip = std::string{""};
            // // auto localIpInfo = server.clientHandler->LocalIp(getip);
            // auto localIpInfo = localIp;
            // json msgj2 = {
            //     {"nodeID",server._NodeID},
            //     {"msgtype","detect"},
            //     {"data",{
            //         {"localip",localIpInfo},
            //         {"localport",localServerPort}
            //     }}
            // };
            // auto msg = msgj2.dump();
            // server.clientHandler->sendTo(remoteServerIp,intRemotePort,msg);
            
            //创建一个定时线程
            // std::thread threadFunc(server.threadhandle);
            
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