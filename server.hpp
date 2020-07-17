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
#include "log.hpp"
#include <queue>

enum msgTypeEnum { noderegister, update, holdpunching };

// for convenience
using json = nlohmann::json;

using namespace boost::asio;

/*udpserver主要的职责：
接收消息
发送消息
*/
class udpServer{
    public:
        //增加一个list用户存储获取到的node的类型

        udpServer(io_service& io_service,
                  std::string rip, 
                  const unsigned short rport,
                  const unsigned short port,
                  bool publicflag,
                  std::string & localIp,
                  std::shared_ptr<std::queue<json>> msgQueuePtr,
                  std::shared_ptr<std::map<std::string,std::string>> pfCkTimeMapPtr,
                  std::shared_ptr<std::map<std::string,unsigned short>> ptickerNumPtr
                  );
        ~udpServer(){};
        std::shared_ptr<ip::udp::socket>  _socketPtr;
        void getLocalIP();
        //存储本地ep的信息
        json _localEpInfo;

        //msgqueue 用于传输层发送消息处理请求
        std::shared_ptr<std::queue<json>> msgQueuePtr;

        //用于发送udp的时候 检查timeout事件的queue
        std::shared_ptr<std::map<std::string,std::string>> fCkTimeMapPtr;
        std::shared_ptr<std::map<std::string,unsigned short>> tickerNumPtr;

    //函数定义
        //udp发送接口，
        void sendTo(std::string &remoteIp,unsigned short &remotePort,const std::string & msg);
        std::string getCurrentTime();
        
    private:

        std::string _localIp;
        std::string _localMappingIp;
        unsigned short _localPort;
        unsigned short _localMappingPort;
        // ip::udp::socket _socket;
        std::array<char,10000> _recvBuffer;
        // std::vector<uint8_t> _testbjson(10000);
        ip::udp::endpoint _remoteEndpoint;


        //每一次启动的时候，放入到自己私有的变量列表中
        std::string _fromCliIp;
        unsigned short _fromCliPort;
        uint _count;
        void startReceive();
        void handleReceive(const boost::system::error_code &error,std::size_t size);
        void primaryCheck(std::string remoteIp,unsigned short remotePort,std::string Msg );
        
};