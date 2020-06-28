#include "server.hpp"
#include <boost/lexical_cast.hpp>
#include <vector>



using namespace boost::asio;
using namespace boost::asio::ip;

udpServer::udpServer(io_service& io_service,const unsigned short port,bool publicflag)
:_socketPtr{std::make_shared<ip::udp::socket>(io_service,ip::udp::endpoint(ip::udp::v4(),port))}
,clientHandler{make_shared<client>(io_service,port,_socketPtr)}
,pulicNodes{}
{
    clientHandler->_clientSocket = _socketPtr;
    //初始化managenode;
    manHost = make_shared<managerHost>();
    // getLocalIP();
    // _socket.bind(udp::endpoint(ip::address().from_string(_localIp),61000));
    std::cout<< _socketPtr->local_endpoint().address().to_string()<<" port: "<<_socketPtr->local_endpoint().port()<<std::endl;
    //放在这里，先发送消息到一个服务器中

    auto getIp = std::string{""};
    _localIp = clientHandler->LocalIp(getIp); //std::string{"192.168.65.2"}; //
    _localPort = port;

    //启动一个timer线程，用于监控是否收到过应答消息
    // std::thread threadFunc(threadhandle);
    //只有public的节点才会启动该线程 将自己放入到publicnode中
    if(publicflag){
        json pubNode = {
            {"ip",_localIp},
            {"port",_localPort}
        };
        //将自己加入到public节点群中
        pulicNodes.insert(pubNode.dump());

        threadPtr = make_shared<thread>([this](){
            std::cout<<"starting timer thread"<<std::endl;
            threadhandle();
        });
    }

    startReceive();
}

void udpServer::startReceive(){
    _recvBuffer.fill('\0');
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

        std::string str(std::begin(_recvBuffer), std::end(_recvBuffer));
        std::cout << "remoteip/port: " <<seeRemoteIp <<"/"<< seeRemotePort << str <<std::endl;
        std::string localIpStr,localPortStr;
        unsigned short localPortInt;
        try
        {
            //检查是一个有效的json字符串
            if(json::accept(_recvBuffer)){
                auto recvJson = json::parse(_recvBuffer);
                std::cout<< "msg+++++++++++++comming " <<  recvJson <<std::endl;
                if(recvJson.find("msgtype") != recvJson.end()){
                    //说明有这个字段，
                    std::string msgType{""};
                    std::vector<std::string> hostInfos;
                    recvJson["msgtype"].get_to(msgType);
                    if (msgType == "detect"){

                        localIpStr = recvJson["data"]["localip"].get<std::string>();
                        localPortStr = recvJson["data"]["localport"].get<std::string>();
                        localPortInt = boost::lexical_cast<unsigned short>(localPortStr);

                        std::cout<<"detect comming:"<< recvJson.dump() <<std::endl;
                        if((seeRemoteIp != localIpStr) || (seeRemotePort != localPortInt) ){
                            std::cout<< "send mapping tyep" << std::endl;
                            
                            //通知另外一个节点，开始探测filtering的行为
                            
                            //分开装
                            json mapping ={
                                {"ip" , seeRemoteIp},
                                {"port", seeRemotePort}
                            };
                            json nodeinfo = {
                                {"ip" , localIpStr},
                                {"port", localPortInt}
                            };
                            //等确定之后装入list中
                            json nodeInfoSee = {
                                {"natmapping", "mapping"},  //true 表明有mapping 行为
                                {"natfiltering",""},  //true 表明有filtering行为
                                {"endpoint",{mapping,nodeinfo}}
                            };
                            // json nodeInfoSeeBody = {
                            //     {"ip" , localIpStr},
                            //     {"port", localPortInt},
                            //     {"natmapping", true},  //true 表明有mapping 行为
                            //     {"natfiltering",false}  //true 表明有filtering行为                            };
                            // };

                            // hostInfos.push_back(nodeInfoSee.dump());
                            auto hostInfos = nodeInfoSee.dump();
                            // hostInfos.push_back(nodeInfoSeeBody.dump());

                            manHost->addNewNode(hostInfos);

                            //存完之后，通知邻居节点来完成filtering的探测
                            //风险，会不会形成封杀隐患
                            //1。获取到邻居的ip和端口号
                            if (pulicNodes.size()>1){
                                auto it = pulicNodes.begin();
                                
                                auto neighber = json::parse(*it);
                                
                                auto ip = neighber["endpoint"][0]["ip"].get<std::string>();
                                auto port = neighber["endpoint"][0]["port"].get<unsigned short>();

                                std::cout << "publicnodes first one: "
                                          << neighber
                                          <<"ip:port"
                                          <<ip
                                          <<port 
                                          << std::endl;
                                //消息的内容是目标的ip地址，肯定是mapping之后的目标
                                json filteringMsgS = {
                                    {"msgtype","filteringcheck"},
                                    {"ip",seeRemoteIp},
                                    {"port",seeRemotePort}
                                };
                                std::string filteringMsg = filteringMsgS.dump();
                                std::cout<<"more than 2 nodes, staring sending :"
                                         << "send: "<<filteringMsg
                                         << std::endl;
                                clientHandler->sendTo(ip,port,filteringMsg);

                            }
                            //2.组装消息发送到对方
                            // manHost.
                            //如果不一样，说明是nat之后，将路由表的信息发送给对方
                            //这里有个问题，就算不一样也无所谓。广播到所有连接我的人
                            // json tst(manHost->_HostList);
                            // json msg = {
                            //     {"msgtype","update"},
                            //     {"data",tst}
                            // };

                            //通知另外一个节点来探索
                            //需要等到该节点明确知道对方是什么nat类型的时候，update到其他的节点中去
                            // for (auto it = manHost->_HostList.begin();it != manHost->_HostList.end();it++){
                            //     json parseit(it->second);
                            //     std::cout<< "test...." <<parseit<<std::endl;
                            //     std::cout<< "test####### "<<parseit[0]<<"type : "<<parseit[0].is_string()<<std::endl;


                            //     auto tarJson = json::parse(parseit[0].get<std::string>());
                            //     auto tarIp = tarJson["ip"].get<std::string>();
                            //     auto tarPort = tarJson["port"].get<unsigned short>();

                            //     // auto msgstr = msg.dump();
                            //     // clientHandler->sendTo(tarIp,tarPort,localPortStr,msgstr);
                            // }
                            // map<std::string,std::vector<std::string>> test;
                            // json tst(test);
                            // clientHandler->sendTo(seeRemoteIp,seeRemotePort,localPortStr);
                            //放入到自己的队列中，然后返回给地方其他人的节点信息，
                        }else{
                            //ip 和 port 都相同
                            std::cout << "====== there all the same =====" << std::endl;
                            std::cout << "remote ip: " <<seeRemoteIp <<" remote port: "<< seeRemotePort<<std::endl;
                            std::cout << "local ip: " <<localIpStr<<" local port: "<< localPortInt <<std::endl;
                            // boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));

                            // clientHandler->sendTo(seeRemoteIp,seeRemotePort,localPortStr);

                            json pubjson = {
                                {"ip",seeRemoteIp},
                                {"port",seeRemotePort}
                            };
                            // auto pubstr = seeRemoteIp + std::to_string(seeRemotePort);

                            
                            json nodeInfoSee = {
                                {"natmapping", "Nonmap"},  //true 表明有mapping 行为
                                {"natfiltering","Nofilter"},  //true 表明有filtering行为
                                {"endpoint",{pubjson}}
                            };

                            auto hostInfos = nodeInfoSee.dump();
                            //说明是公网Ip 或者是恶意节点 此时增加到publicnode中
                            //对这个群组的数量进行限定 暂定为3个吧
                            std::cout << "___________" <<pulicNodes.size() << std::endl;
                            if( 3 >  pulicNodes.size()){
                                pulicNodes.insert(nodeInfoSee.dump());
                            }
                            std::cout << "___________" <<pulicNodes.size() << std::endl;

                            manHost->addNewNode(hostInfos);
                            // manHost.
                            //如果不一样，说明是nat之后，将路由表的信息发送给对方
                            //这里有个问题，就算不一样也无所谓。广播到所有连接我的人
                            json tst(manHost->_HostList);

                            json msg = {
                                {"msgtype","update"},
                                {"data",tst}
                            };
                            for (auto it = manHost->_HostList.begin();it != manHost->_HostList.end();it++){
                                auto tarEndPoint = json::parse(it->second);
  
                                std::cout<< "test...." <<tarEndPoint<<std::endl;

                                auto tarIpStr = tarEndPoint["endpoint"][0]["ip"].get<std::string>();
                                auto tarPortInt = tarEndPoint["endpoint"][0]["port"].get<unsigned short>();

                                auto msgstr = msg.dump();
                                std::cout<< "starting send...." <<msgstr<<std::endl;
                                clientHandler->sendTo(tarIpStr,tarPortInt,msgstr);
                            }
                        }

                    }else if (msgType == "update"){
                        std::cout<<"update comming"<<std::endl;
                        //update 消息成功收到，然后存到本地 开始发送消息到各个节点，有几个发几个，这里并发的发 逐个发
                        std::cout<< "update...." <<recvJson["data"]<<std::endl;
                        //遍历所有的endpoint 除了自己，然后相连
                                     
                        for (auto it = recvJson["data"].begin();it != recvJson["data"].end();it++){

                            //map 的字符串只能用着样的方式进行转换
                            //先转成json 然后获取string 在转成json 真麻烦
                            json item(*it);
                            auto itemobj = json::parse(item.get<std::string>());

                            auto tarIp = itemobj["endpoint"][0]["ip"].get<string>();
                            auto tarPort = itemobj["endpoint"][0]["port"].get<unsigned short>();
                            //排除自己的ip
                            if (tarIp == _localIp){
                                std::cout<<"yourself...."<<std::endl;
                                continue;
                            }
                            //否则与其他的节点相连
                            json msg = {
                                {"msgtype","connect"},
                                {"data","justtest"} //这里应该是开始连接，根据不同节点的nat类型开始连接
                            };
                            auto msgstr = msg.dump();
                            std::cout<< "start sending connect msg...." <<msgstr<<std::endl;
                            // auto localPortStr = std::to_string(_localPort);
                            clientHandler->sendTo(tarIp,tarPort,msgstr);
                        }
                    }else if (msgType == "connect"){
                        std::cout<<"holdpunching comming"<<std::endl;

                    }else if(msgType == "filteringcheck"){ //走filtering 行为检查
                        std::cout<<"filteringcheck comming"<<std::endl;

                        //需要做一次保护，只能pulicnodes的节点可以发
                        localIpStr = recvJson["ip"].get<std::string>();
                        localPortInt = recvJson["port"].get<unsigned short>();
                        
                        json filMsg = {
                            {"msgtype","callclient"},
                            {"data","areyouok"}
                        };
                        auto filMsgStr = filMsg.dump();

                        clientHandler->sendTo(localIpStr,localPortInt,filMsgStr);
                        //增加一个定时器 异步等待是否会收到信息
                        auto keyStr = localIpStr + std::to_string(localPortInt);
                        //获取当前时间
                        std::time_t curTime = std::time(nullptr);
                        fCkTimeMap.insert(std::pair<std::string,std::time_t>(keyStr,curTime));
                        
                    }else if(msgType == "callclient"){
                        std::cout<<"callclient comming"<<std::endl;
                        //只要收到该类型的消息就可以，不用考虑内容。
                        //找到除了自己的所有邻居发广播
                        json filteringMsgR = {
                                    {"msgtype","callclientRes"},
                                    {"ip",seeRemoteIp},
                                    {"port",seeRemotePort}
                        };
                        std::string nIp;
                        unsigned short nPort;
                        for (auto it = pulicNodes.begin();it != pulicNodes.end();it ++ ){
                            auto neighRes = json::parse(*it);
                            nIp = neighRes["ip"].get<std::string>();
                            nPort = neighRes["port"].get<unsigned short>();


                            if (nIp == _localIp){
                                std::cout<< " myself when sending filteringCheck"<<std::endl; 
                                continue;
                            }

                            auto filteringMsgRstr = filteringMsgR.dump();
                            clientHandler->sendTo(nIp,nPort,filteringMsgRstr);
                        }

                    }else if (msgType == "callclientRes"){
                        std::cout << "callclientRes is comming" <<std::endl;
                        localIpStr = recvJson["ip"].get<std::string>();
                        localPortInt = recvJson["port"].get<unsigned short>();
                        auto ipKey =  localIpStr + std::to_string(localPortInt);
                        //查询本地是否有该节点，如果有
                        auto findit = manHost->_HostList.find(ipKey);
                        if(findit !=  manHost->_HostList.end()){
                            //说明有，则更新下数据
                            auto findmsg = json::parse(findit->second);
                            findmsg["natfiltering"] = "NonFilter";
                            manHost->_HostList.at(ipKey) = findmsg.dump(); // 更新filtering的数据

                            //同时踢出timer中的记录
                            auto getIt = fCkTimeMap.find(ipKey);
                            if (fCkTimeMap.end() != getIt){
                                //说明在20之内收到了回复，则直接删除该记录
                                fCkTimeMap.erase(getIt);
                            }
                        }
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

//用于定时
void udpServer::threadhandle(){
    std::time_t curTimeForCh;
    std::map<std::string,std::string>::iterator finditkey;
    json findmsg;
    while(true){
        //sleep 5 s之后做处理
        std::this_thread::sleep_for(std::chrono::seconds(5));
        //检查list中的发送探测的节点id
        //对比当前时间与发送时间的间隔，如果超过预设的值，则踢出，并修改状态，认为不同
        std::cout<<"checking filtering size:"<<fCkTimeMap.size()<<std::endl;
        curTimeForCh = std::time(nullptr);
        for(auto it = fCkTimeMap.begin();it != fCkTimeMap.end();){
            std::cout<<"current checking .... "<< it->first << it->second <<std::endl;
            //20s timeout 时间，如果没有收到，则认为不再会收到
            if(curTimeForCh - it->second > 20){
                std::cout<<"node Id: "<<it->first << " have remain for more than 20s"<<std::endl;
                //修改node信息
                finditkey = manHost->_HostList.find(it->first);
                if(finditkey !=  manHost->_HostList.end()){
                    //说明有，则更新下数据
                    findmsg = json::parse(finditkey->second);
                    std::cout<< "checking filtering....findmsg"
                             <<findmsg
                             <<findmsg.is_object()
                             <<findmsg.is_string()
                             <<std::endl;
                    findmsg["natfiltering"] = "NonFilter";
                    manHost->_HostList.at(it->first) = findmsg.dump(); // 更新filtering的数据
                }
                //删除这一条
                std::cout<< "erase it..." <<it->first << ".." <<it->second <<std::endl;
                fCkTimeMap.erase(it++);
                

            }else{
                //print map info....
                std::cout<< it->first << ".." <<it->second <<std::endl;
                it++;
            }

            
        }
    }
}

std::string udpServer::genUuid(){
    random_generator gen;
    uuid id = gen();
    return to_string(id);
}