#include "server.hpp"
#include <boost/lexical_cast.hpp>
#include <vector>

#define PACKAGENUM 60

using namespace boost::asio;
using namespace boost::asio::ip;

udpServer::udpServer(
                        io_service& io_service,
                        std::string rip,
                        const unsigned short rport,
                        const unsigned short port,
                        bool publicflag,
                        std::string & localIp,
                        std::shared_ptr<std::queue<json>> pmsgQueuePtr,
                        std::shared_ptr<std::map<std::string,std::string>> pfCkTimeMapPtr,
                        std::shared_ptr<std::map<std::string,unsigned short>> ptickerNumPtr
                    )
:_socketPtr{std::make_shared<ip::udp::socket>(io_service,ip::udp::endpoint(ip::udp::v4(),port))}
,msgQueuePtr{pmsgQueuePtr}
,fCkTimeMapPtr{pfCkTimeMapPtr}
,tickerNumPtr{ptickerNumPtr}
// ,tickerNodes{}
// ,publicFlag{publicflag}
// ,_NodeID{}
// ,_fromCliIp{rip}
// ,_fromCliPort{rport}
,_count{0}
{// ,q("Phillip's Demo Dispatch Queue", 
    std::cout<< _socketPtr->local_endpoint().address().to_string()<<" port: "<<_socketPtr->local_endpoint().port()<<std::endl;
    startReceive();
}

void udpServer::startReceive(){
    _recvBuffer.fill('\0');
    // _testbjson;
    _socketPtr->async_receive_from(
        boost::asio::buffer(_recvBuffer),
        _remoteEndpoint,
        boost::bind(&udpServer::handleReceive,this,boost::asio::placeholders::error,boost::asio::placeholders::bytes_transferred)
    );
}

void udpServer::handleReceive(const boost::system::error_code &error,std::size_t size){


    if (!error || error == boost::asio::error::message_size){
        auto teststr =  _recvBuffer;
        auto tmprEp = _remoteEndpoint;
        
        // q.dispatch([=](){ //&表示捕获所enclosing variables by reference  用= 拷贝的方式
        //假设已经收到了string的数据，
        //收到有效消息之后，获取到对端的ip 和 port 
        auto seeRemoteIp = tmprEp.address().to_string();
        auto seeRemotePort = tmprEp.port();

        std::string str(std::begin(teststr), std::end(teststr));
        // if (seeRemoteIp == _localIp) return;
        //收到的数据不是本节点发过来的，则才处理
        primaryCheck(seeRemoteIp,seeRemotePort,str);

        // });

        startReceive();
    }
}

void udpServer::sendTo(std::string &remoteIp,unsigned short &remotePort,const std::string & msg){

   try{
        
        auto remoteEndPoint = boost::asio::ip::udp::endpoint(ip::address().from_string(remoteIp),remotePort);
        boost::system::error_code ignored_error;
        //why need open
        // bindSocket.open(boost::asio::ip::udp::v4());
        auto timestr = getCurrentTime();
        std::cout<<"=="<<timestr<<"start sending:"<<remoteIp<<" port:"<<remotePort<<"msg:"<<msg<<std::endl;

        boost::system::error_code errCode;
        // auto testjson = json::parse(msg);
        // std::vector<std::uint8_t> bjsontst = json::to_bson(testjson);
        _socketPtr->send_to(buffer(msg),remoteEndPoint,0,errCode);
        
        std::cout<<"finished the sending,error:"<<errCode<<std::endl;

    }
    catch(const std::exception& e)
    {
        std::cerr << "Got errors: " <<e.what() << '\n';
      
    }    
}

void udpServer::getLocalIP(){
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

//作为过滤器，将不同的消息类型放入到不同的消息处理线程队列中
void udpServer::primaryCheck(std::string remoteIp,unsigned short remotePort,std::string Msg ){
    std::cout <<"=="<<getCurrentTime()<<"=="<< ".....remoteip/port: " <<remoteIp <<"/"<< remotePort << Msg <<std::endl;
    //过滤掉自己的ip发来的消息，
    //解析消息类型，client ======detectmapping==> server1 
    //            server1_port1==updatepublic==>client 
    //            server1_port2================>client 
    //            server1_port1==helpdetect====>server2_port1
    //            server2_port1================>client
    //            server2_port================>client
    //                   ==> updatepublic ==>
    //            detectfiltering ==> 
    //            connect ==> updatenat 
    //           
    if(remoteIp != _localIp){
        //
        std::string localIpStr,localPortStr;
        unsigned short localPortInt;
        try
        {
            //检查是一个有效的json字符串
            auto teststr =  _recvBuffer;
            if(json::accept(teststr)){
                auto recvJson = json::parse(teststr);
                recvJson["remoteip"] = remoteIp;
                recvJson["remoteport"] = remotePort;
                //将数据放入到队列中 队列ptr不能为空
                if(msgQueuePtr != nullptr){
                    msgQueuePtr->push(recvJson);
                }else{
                    std::cout << "error: msgQueuePtr is null"<<std::endl; 
                }
            }else{
                //收到的消息为异常消息，不是标准的json格式，
                std::cout<< "=+=+=+=+==+=+=+=+==+=+=+ bad msg "<<_recvBuffer<<std::endl;
            }
        }
        catch(const std::exception& e)
        {
            // output exception information
            std::cout << "message: " << e.what() << '\n';
            abort();
            // << "exception id: " << e.id << '\n'
            // << "byte position of error: " << e.byte << std::endl;
        }
    }else{
        //来自本地ip的这里暂时不处理，但是分支保留便于分析
        //todo
    }
}


std::string udpServer::getCurrentTime(){
    std::time_t curTime = std::time(nullptr);
    tm*  t_tm = localtime(&curTime);
    std::string removeReturn = asctime(t_tm);
    removeReturn.pop_back();
    return removeReturn;
}