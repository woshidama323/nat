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
#include <queue>

//产生uuid
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
using namespace boost::uuids;


enum msgTypeEnum { noderegister, update, holdpunching };

// for convenience
using json = nlohmann::json;

/*
Node 为一个p2p对等网络中的一个节点抽象
组件：
    udpserver：负责管理udpserver 发送接收外部过来的udp包 
    hostlist ：所有邻居节点（可以直接连接的所有node列表）
    tickerMap：保持心跳的列表
    定时线程   ：用于计时功能，满足定时心跳，timeout检查等功能
    node相关信息：
             nodeid
    连接策略   ： 
*/
class Node{
    public:
    //变量列表：

        
        //node id 用于标识网络中的不同节点
        std::string _NodeID;


        //msgqueue 用于传输层发送消息处理请求
        std::shared_ptr<std::queue<json>> msgQueuePtr;
        //map 用于存发送filtering探测的时间点，用于timeout操作
        //发送探测请求后回应的timeout表，一定时间内包是否接收到
        //需要用智能指针，因为后期要传给子模块中的udpserver部分
        std::shared_ptr<std::map<std::string,std::string>> fCkTimeMapPtr;
        //  fCkTimeMap;

        //ticker的数量，用于标识两个节点之间心跳连接情况，如果丢失一定数量的心跳，则认为是断联，清除出去
        std::shared_ptr<std::map<std::string,unsigned short>> tickerNumPtr;
        // std::map<std::string,unsigned short> tickerNum;

        //心跳列表 set 中不会有重复的节点
        std::set<std::string> tickerNodes;

        //bootstrap node
        std::set<std::string> pulicNodes;

        //心跳返回队列
        std::map<std::string,std::time_t> tickerNodeRes;

        //线程指针
        std::shared_ptr<thread> threadPtr;
       //线程指针
        std::shared_ptr<thread> threadPtr2;

        //is bootstrap flag,用于决定当前启动的节点是否为bootstrap节点 一般为公网节点
        bool publicFlag;

        //相邻（在连接状态）节点列表
        std::shared_ptr<managerHost> manHostPtr;

        //udp 服务放到最后的主要目的是其他的要在它之前初始化
        std::shared_ptr<udpServer> udpServerPtr;

        std::shared_ptr<udpServer> udpHelpServerPtr;

        //master port 
        unsigned short _masterPort;

        //slave port 
        unsigned short _slavePort;

    //函数列表：

        //构造函数 
        Node(io_service& io_service,std::string rip,const unsigned short rport,const unsigned short port,bool publicflag,std::string & localIp);

        ~Node();

        //获取当前时间
        std::string getCurrentTime();

        //显示消息队列的内容，用于debug
        void listMsgQueue();

        //更新路由表信息
        void updateRouteInfo();
        
        //产生uuid,用于设置node的id 
        std::string genUuid();

        //线程处理函数
        void threadhandle();

        //消息处理函数
        void msgThread();

        //以上消息函数是否可以用dispatch queue来完成？
        void MsgProcess(json recvJson);

        //发送探测请求
        void startDetect();


        //get public nodes
        std::string getPublicNodes();
        //更新public节点信息
        void updatePublicNode();

        //更新非public节点信息
        void updateNonPublicNode();

        //增加一个start接口用于开始一些初始操作
        void start();

        
        

    private:
        std::string _localIp,_fromCliIp;
        unsigned short _localPort,_fromCliPort;  
        json _localEpInfo;
        int _count;

        

};


/*
dataobj = 
{   "natmapping":""
    "natfiltering":""
    endpoint{}
}
消息结构json格式
    msgtype : detectfiltering 
    data    : {dataobj} 
    //json数组
    msgId   :   xxxx
    nodeId  :   xxx
    premsgid: 

*/