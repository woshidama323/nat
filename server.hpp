
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/algorithm/string.hpp>
#include "json.hpp"
#include <iostream>
#include "managerhost.hpp"
#include "client.hpp"
#include <set>

#include <chrono>
#include <thread> 

//获取时间
#include <ctime>

//dispatch queue
#include "dispatchqueue.hpp"

//产生uuid
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
using namespace boost::uuids;


enum msgTypeEnum { noderegister, update, holdpunching };

// for convenience
using json = nlohmann::json;

using namespace boost::asio;

class udpServer{
    public:
        //增加一个list用户存储获取到的node的类型

        udpServer(io_service& io_service,const unsigned short port,bool publicflag);
        ~udpServer(){};
        std::shared_ptr<managerHost> manHost;
        std::shared_ptr<client> clientHandler;
        // udpServer();
        std::shared_ptr<ip::udp::socket>  _socketPtr;
        void getLocalIP();

        std::set<std::string> pulicNodes;
        
        //心跳列表
        std::set<std::string> tickerNodes;

        //心跳返回队列
        std::map<std::string,std::time_t> tickerNodeRes;

        void threadhandle();
        void msgThread();

        //map 用于存发送filtering探测的时间点，用于timeout操作
        std::map<std::string,std::string> fCkTimeMap;

        //产生uuid 
        std::string genUuid();

        //update all the routing info
        void updateRouteInfo();

        std::shared_ptr<thread> threadPtr,threadPtr2;

        dispatch_queue q;
        
    private:

        std::string _localIp;
        unsigned short _localPort;
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
