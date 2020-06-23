
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include "json.hpp"
#include <iostream>
#include "managerhost.hpp"
#include "client.hpp"


enum msgTypeEnum { noderegister, update, holdpunching };

// for convenience
using json = nlohmann::json;

using namespace boost::asio;

class udpServer{
    public:
        //增加一个list用户存储获取到的node的类型

        udpServer(io_service& io_service,const unsigned short port);
        ~udpServer(){};
        std::shared_ptr<managerHost> manHost;
        std::shared_ptr<client> clientHandler;
        // udpServer();
        std::shared_ptr<ip::udp::socket>  _socketPtr;
        void getLocalIP();
        
    private:

        std::string _localIp;
        // ip::udp::socket _socket;
        std::array<char,10000> _recvBuffer;
        ip::udp::endpoint _remoteEndpoint;
        void startReceive();
        void handleReceive(const boost::system::error_code &error,std::size_t size);
        void sendRequest(std::string msg, const std::string & desIp, const unsigned short desPort);
        
};

/*
客户端请求的类型有哪一些：
消息的数据结构（用json的方式进行）
消息类型：

注册消息
{
    "msgtype":"register"
    "data":{
        "from":{
            "ip":"xxx",
            "port":"xxx",
            "nattype":"",
        }
    }
}

更新数据
{
    "msgtype":"update"
    "data":{
        "from":{
            "ip":"xxx",
            "port":"xxx",
            "nattype":"",
        }
    }
}

打洞请求
{
    "msgtype":"holdpunching"
    "data":{
        "from":{
            "ip":"xxx",
            "port":"xxx",
            "nattype":"",
        },
        "to":{
            "ip":"xxx",
            "port":"x",
            nattype:"xxx",
        },
    }
}

1. 新节点注册请求

2. 目标节点的连接请求
3. 
*/