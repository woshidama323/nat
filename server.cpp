#include "server.hpp"
#include <boost/lexical_cast.hpp>
#include <vector>



using namespace boost::asio;
using namespace boost::asio::ip;

udpServer::udpServer(io_service& io_service,const unsigned short port,bool publicflag)
:_socketPtr{std::make_shared<ip::udp::socket>(io_service,ip::udp::endpoint(ip::udp::v4(),port))}
,clientHandler{make_shared<client>(io_service,port,_socketPtr)}
,pulicNodes{}
,tickerNodes{}
,q("Phillip's Demo Dispatch Queue", 4)
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
        threadPtr2 = make_shared<thread>([this](){
            std::cout<<"starting msg thread"<<std::endl;
            msgThread();
        });
    }

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

        // std::vector<uint8_t> myvector(teststr.begin(), teststr.end());
        // std::cout<< json::parse(_testbjson.begin(),_testbjson.end()) <<std::endl;

        // auto getjson = json::from_bson(_testbjson);

        auto tmprEp = _remoteEndpoint;
        
        q.dispatch([=](){ //&表示捕获所enclosing variables by reference  用= 拷贝的方式
            //假设已经收到了string的数据，
            //收到有效消息之后，获取到对端的ip 和 port 
            auto seeRemoteIp = tmprEp.address().to_string();
            auto seeRemotePort = tmprEp.port();

            std::string str(std::begin(teststr), std::end(teststr));
            std::cout << ".....remoteip/port: " <<seeRemoteIp <<"/"<< seeRemotePort << str <<std::endl;
            std::string localIpStr,localPortStr;
            unsigned short localPortInt;
            try
            {
                //检查是一个有效的json字符串
                if(json::accept(teststr)){
                    auto recvJson = json::parse(teststr);
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

                                manHost->addNewNode(nodeInfoSee.dump());

                                //自己也要加入进去
                                json nodeinfoself = {
                                    {"ip" , _localIp},
                                    {"port", _localPort}
                                };
                                //等确定之后装入list中
                                json nodeInfoSeeSelf = {
                                    {"natmapping", "Nonmap"},  //true 表明有mapping 行为
                                    {"natfiltering","Nonfilter"},  //true 表明有filtering行为
                                    {"endpoint",{nodeinfoself}}
                                };

                                manHost->addNewNode(nodeInfoSeeSelf.dump());
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
                                    auto msgID = genUuid();
                                    json filteringMsgS = {
                                        {"msgtype","filteringcheck"},
                                        {"ip",seeRemoteIp},
                                        {"port",seeRemotePort},
                                        {"msgid",msgID}
                                    };
                                    std::string filteringMsg = filteringMsgS.dump();
                                    std::cout<<"more than 2 nodes, staring sending :"
                                            << "send: "<<filteringMsg
                                            << std::endl;
                                    clientHandler->sendTo(ip,port,filteringMsg);
                                    //获取当前时间
                                    std::time_t curTime = std::time(nullptr);
                                    //增加一个定时器 异步等待是否会收到信息
                                    auto keyStr = "filtering_" + to_string(curTime) + "_" + localIpStr + "_" + std::to_string(localPortInt);
                                
                                    fCkTimeMap.insert(std::pair<std::string,std::string>(genUuid(),keyStr));

                                }

                            }
                            else{
                                //ip 和 port 都相同
                                std::cout << "====== there all the same =====" << std::endl;
                                std::cout << ".....local ip: " <<localIpStr<<" local port: "<< localPortInt <<std::endl;
                                json pubjson = {
                                    {"ip",seeRemoteIp},
                                    {"port",seeRemotePort}
                                };
                                json nodeInfoSee = {
                                    {"natmapping", "Nonmap"},  //true 表明有mapping 行为
                                    {"natfiltering","Nonfilter"},  //true 表明有filtering行为
                                    {"endpoint",{pubjson}}
                                };
                                auto hostInfos = nodeInfoSee.dump();
                                manHost->addNewNode(hostInfos);
                                                                //自己也要加入进去
                                json nodeinfoself = {
                                    {"ip" , _localIp},
                                    {"port", _localPort}
                                };
                                //等确定之后装入list中
                                json nodeInfoSeeSelf = {
                                    {"natmapping", "Nonmap"},  //true 表明有mapping 行为
                                    {"natfiltering","Nonfilter"},  //true 表明有filtering行为
                                    {"endpoint",{nodeinfoself}}
                                };

                                manHost->addNewNode(nodeInfoSeeSelf.dump());
                                // manHost.
                                //如果不一样，说明是nat之后，将路由表的信息发送给对方
                                //这里有个问题，就算不一样也无所谓。广播到所有连接我的人
                                updateRouteInfo();
                            }

                        }
                        else if (msgType == "update"){
                            //专用于更新节点信息用
                            std::cout<<"update comming"<<std::endl;
                            //update 消息成功收到，然后存到本地 开始发送消息到各个节点，有几个发几个，这里并发的发 逐个发
                            std::cout<< "update...." <<recvJson["data"]<<std::endl;
                            //遍历所有的endpoint 除了自己，然后相连
                            //更新本地的routing表，然后连接新的节点 
                            // manHost->_HostList
                            auto selfEp = _localIp + "_" + to_string(_localPort);
                            auto getself = recvJson["data"].find(selfEp);
                            if (getself != recvJson["data"].end()){
                                //找到自己，将自己的信息放入到本地
                                _localEpInfo = getself.value();
                            }    

                            for (auto it = recvJson["data"].begin();it != recvJson["data"].end();it++){

                                //map 的字符串只能用着样的方式进行转换
                                //先转成json 然后获取string 在转成json 真麻烦
                                std::cout << "update process.........."<<*it<< std::endl;
                                json item(*it);
                                auto itemobj = json::parse(item.get<std::string>());

                                auto tarIp = itemobj["endpoint"][0]["ip"].get<string>();
                                auto tarPort = itemobj["endpoint"][0]["port"].get<unsigned short>();                          
                                auto checkKey = tarIp + "_" +to_string(tarPort);
                                if (manHost->_HostList.find(checkKey) != manHost->_HostList.end()){
                                    //本地已经有了这个一个目标了，直接跳过
                                    std::cout<< "+++++++ does exist in current list: "<<checkKey<<std::endl;
                                    continue;
                                }

                                //否则与其他的节点相连
                                json msg = {
                                    {"msgtype","openconnect"},
                                    {"data",_localEpInfo} //这里应该是开始连接，根据不同节点的nat类型开始连接
                                };
                                //根据对方的类型来决定如何连接
                                //1. 是public的
                                if (itemobj["natmapping"].get<std::string>() == "Nonmap" && itemobj["natfiltering"].get<std::string>() == "Nonfilter"){
                                    std::cout<< "start sending connect msg...." <<msg<<std::endl;
                                    // auto localPortStr = std::to_string(_localPort);
                                    //如果明确是public的则加入到public中用于探测
                                    auto pub = tarIp + "_" + to_string(tarPort);
                                    pulicNodes.insert(pub);
                                    
                                }else {
                                    json msgproxy = {
                                        {"msgtype","proxyconnect"},
                                        {"data",itemobj.dump()} //这里应该是开始连接，根据不同节点的nat类型开始连接
                                    };
                            
                                    std::cout<< "start sending proxyconnect msg...." <<msgproxy<<std::endl;
                                    // auto localPortStr = std::to_string(_localPort);
                                    //问题 发给proxy端的ip port应该从哪里找？是public的节点，这些节点确定有目标地址和port么
                                    //从自己的队列中的public
                                    if (pulicNodes.size()>1){
                                        auto it = pulicNodes.begin();
                                        
                                        auto neighber = json::parse(*it);
                                        
                                        auto ip = neighber["endpoint"][0]["ip"].get<std::string>();
                                        auto port = neighber["endpoint"][0]["port"].get<unsigned short>();
                                        clientHandler->sendTo(ip,port,msgproxy.dump());
                                    }                                
                                }
                                clientHandler->sendTo(tarIp,tarPort,msg.dump());

                            }
                        }
                        else if (msgType == "proxyconnect")
                        {
                            std::cout<<"holdpunching comming"<<std::endl;
                            auto tarIp = recvJson["data"]["tarIp"].get<std::string>();
                            auto tarPort = recvJson["data"]["tarIp"].get<unsigned short>();
                            //有人请求帮忙，解析body中的ip和端口号，然后通知双端
                            
                            json msg = {
                                {"msgtype","helpconnect"},
                                {"data",recvJson["data"].dump()} //这里应该是开始连接，根据不同节点的nat类型开始连接
                            };

                            clientHandler->sendTo(tarIp,tarPort,msg.dump());
                            

                        }
                        else if(msgType == "helpconnect"){
                            std::cout<<"help connect is comming"<<std::endl;
                            auto tarIp = recvJson["data"]["tarIp"].get<std::string>();
                            auto tarPort = recvJson["data"]["tarIp"].get<unsigned short>();

                            // //将这个节点加入到我的队列中
                            // json nodeInfoSee = {
                            //     {"natmapping", "mapping"},  //true 表明有mapping 行为
                            //     {"natfiltering",""},  //true 表明有filtering行为
                            //     {"endpoint",{mapping,nodeinfo}}
                            // };
                            // manHost->addNewNode(hostInfos);
                            json msg = {
                                {"msgtype","openconnect"},
                                {"data",recvJson["data"].dump()} //这里应该是开始连接，根据不同节点的nat类型开始连接
                            };

                            clientHandler->sendTo(tarIp,tarPort,msg.dump());
                            // 每个人拿到的都是对端的mapping之后的ip port
                            //收到帮助请求之后，收到请求之后，向对端发送连接请求
                            //可能是发送的应答也可能是其他，增加一个msg的id号来区分
                            

                        }
                        else if (msgType == "openconnect"){
                            std::cout<<"-----------------pouching successful "
                                    << "ip: "
                                    << seeRemoteIp
                                    << " port: "
                                    << seeRemotePort
                                    << "data is :"
                                    << recvJson["data"].is_object()
                                    << recvJson["data"].is_string()
                                    << recvJson["data"].is_array()
                                    <<std::endl;
                            
                            //开始建立心跳，定时发送信息，加入到心跳队列中
                            //如果对方是public的则直接相连,
                            //则当filtering来处理  收到该请求，返回对方应答
                            auto tarEpStr = seeRemoteIp + "_" + to_string(seeRemotePort) ;
                            // tickerNodes.insert(tarEpStr);
                            //加入到自己的list中，心跳的来源也是从这里来的
                            //当收到connect的连接的时候，要相应，如果相应
                            manHost->addNewNode(recvJson["data"]);
                        }
                        else if(msgType == "filteringcheck"){ //走filtering 行为检查
                            std::cout<<"filteringcheck comming"<<std::endl;

                            //需要做一次保护，只能pulicnodes的节点可以发
                            localIpStr = recvJson["ip"].get<std::string>();
                            localPortInt = recvJson["port"].get<unsigned short>();
                            
                            json filMsg = {
                                {"msgtype","callclient"},
                                {"ip",localIpStr},
                                {"port",localPortInt},
                                {"originip",seeRemoteIp},
                                {"originport",seeRemotePort},
                                {"msgid",recvJson["msgid"].get<std::string>()}
                            };
                            clientHandler->sendTo(localIpStr,localPortInt,filMsg.dump());

                        }
                        else if(msgType == "callclient"){
                            std::cout<<"callclient comming"<<std::endl;
                            //只要收到该类型的消息就可以，不用考虑内容。
                            //找到除了自己的所有邻居发广播
                            json filteringMsgR = {
                                        {"msgtype","callclientRes"},
                                        {"ip",seeRemoteIp},
                                        {"port",seeRemotePort},
                                        {"msgid",recvJson["msgid"].get<std::string>()}
                            };
                            std::string nIp;
                            unsigned short nPort;

                            //知道自己是什么类型了，更新下，同时上报到之前的节点
                            auto oIp = recvJson["originip"].get<std::string>();
                            auto oPort = recvJson["originport"].get<unsigned short>();
                            clientHandler->sendTo(oIp,oPort,filteringMsgR.dump());
                            //这个时候要知道自己的状态是什么 ？什么样的nat类型
                            //知道自己的类型，修改
                            // json mapping ={
                            //     {"ip" , seeRemoteIp},
                            //     {"port", seeRemotePort}
                            // };
                            // json nodeinfo = {
                            //     {"ip" , localIpStr},
                            //     {"port", localPortInt}
                            // };
                            // //等确定之后装入list中
                            // json nodeInfoSee = {
                            //     {"natmapping", "mapping"},  //true 表明有mapping 行为
                            //     {"natfiltering",""},  //true 表明有filtering行为
                            //     {"endpoint",{mapping,nodeinfo}}
                            // };
                            // // manHost->changeNode()


                        }
                        else if (msgType == "callclientRes"){
                            std::cout << "callclientRes is comming" <<std::endl;
                            localIpStr = recvJson["ip"].get<std::string>();
                            localPortInt = recvJson["port"].get<unsigned short>();
                            auto ipKey =  localIpStr + "_" +  std::to_string(localPortInt);
                            //查询本地是否有该节点，如果有
                            auto findit = manHost->_HostList.find(ipKey);
                            if(findit !=  manHost->_HostList.end()){
                                //说明有，则更新下数据
                                auto findmsg = json::parse(findit->second);
                                findmsg["natfiltering"] = "NonFilter";
                                manHost->_HostList.at(ipKey) = findmsg.dump(); // 更新filtering的数据

                                //同时踢出timer中的记录
                                auto getIt = fCkTimeMap.find(recvJson["msgid"].get<std::string>());
                                if (fCkTimeMap.end() != getIt){
                                    //说明在20之内收到了回复，则直接删除该记录
                                    fCkTimeMap.erase(getIt);
                                }
                                //此时需要向所有的邻居更新下该信息

                            }
                            //在经过proxy确认之后的nat的节点，对方是主动连过来的。这个时候更新下
                            updateRouteInfo();
                        }
                        else if (msgType == "ticker"){
                            std::cout<<"from "<<seeRemoteIp<<"xx"<<seeRemotePort<<" is comming xxx "<<std::endl;
                            auto msgID = recvJson["msgid"].get<std::string>();
                            json msgRes = {
                                {"msgtype","tickerRes"},
                                {"msgid",msgID}
                            };
                            clientHandler->sendTo(seeRemoteIp,seeRemotePort,msgRes.dump());
                        }
                        else if (msgType == "tickerRes"){
                            //收到ticker相应之后才能确定该节点依然在线
                            auto msgID = recvJson["msgid"].get<std::string>();
                            std::cout<< "? ??   ?????? ? ?get msgid:" <<msgID <<std::endl;
                            
                            auto it = fCkTimeMap.find(msgID);
                            if (it != fCkTimeMap.end()){
                                fCkTimeMap.erase(it);
                            }
                            //同时将该节点加入到自己的nodelist中，只有连接成功之后的节点可以加入

                            
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
        });


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
    json findmsg,tickermsg,msgID;
    std::vector<std::string> results;
    unsigned short port;
    time_t timePoint;
    bool needNotify = false;
    std::string key;
    while(true){
        //sleep 5 s之后做处理
        std::this_thread::sleep_for(std::chrono::seconds(5));
        //检查list中的发送探测的节点id
        //对比当前时间与发送时间的间隔，如果超过预设的值，则踢出，并修改状态，认为不同
        std::cout<<"checking msg res size:"<<fCkTimeMap.size()<<std::endl;
        curTimeForCh = std::time(nullptr);

        //判断nat的类型 //同时兼顾检测是否收到心跳的消息响应 统一一下是，都是等待消息回应，只是收到之后的action不一样而已
        for(auto it = fCkTimeMap.begin();it != fCkTimeMap.end();){
            std::cout<<"current checking .... "<< it->first << it->second <<std::endl;
            //20s timeout 时间，如果没有收到，则认为不再会收到，如果是filtering 的消息回复，则修改响应的值，然后通知，如果是ticker的回复，删除该跳记录，然后通知
            //msgid msgtype_time_ip_port  23gadsff -> filtering_152389_172.0.0.1_60001
            boost::algorithm::split(results, it->second, boost::is_any_of("_"));
            timePoint = std::stoi(results[1]);
            auto hostKey = results[2] + "_" +results[3];
            if(curTimeForCh - timePoint > 20){
                //不知道当前是什么类型的消息,但是此时此刻还在map中，说明没有收到回复，做响应的处理
                if(results[0] == "filtering"){
                    finditkey = manHost->_HostList.find(hostKey);
                    if(finditkey !=  manHost->_HostList.end()){
                        //说明有，则更新下数据
                        findmsg = json::parse(finditkey->second);
                        std::cout<< "checking filtering....findmsg"
                                <<findmsg
                                <<findmsg.is_object()
                                <<findmsg.is_string()
                                <<std::endl;
                        findmsg["natfiltering"] = "isFilter";
                        manHost->_HostList.at(it->first) = findmsg.dump(); // 更新filtering的数据
                    }
                    //删除这一条
                    std::cout<< "erase it..." <<it->first << ".." <<it->second <<std::endl;
                    fCkTimeMap.erase(it++);

                    //通知所有的节点更新数据
                    needNotify = true;

                }
                else if(results[0] == "ticker"){
                    needNotify = true;
                    finditkey = manHost->_HostList.find(hostKey);
                    if(finditkey != manHost->_HostList.end()){
                        manHost->_HostList.erase(finditkey);
                    }
                    fCkTimeMap.erase(it++);
                }
            }else{
                //print map info....
                std::cout<< it->first << ".." <<it->second <<std::endl;
                ++it;
            }

            results.clear();
        }

        //增加一个节点的事件检查，加入和退出
        for(auto tit = manHost->_HostList.begin();tit != manHost->_HostList.end();tit++){
            std::cout<< ".........ticker for target: "
                     << tit->first 
                     << ".."
                     << tit->second
                     << std::endl;
            boost::algorithm::split(results, tit->first, boost::is_any_of("_"));
            port = std::stoi( results[1] );
            auto msgID = genUuid();
            tickermsg = {
                {"msgtype","ticker"},
                {"data","hello"},
                {"msgid",msgID}
            };
            if(results[0] == _localIp) continue;
            clientHandler->sendTo(results[0],port,tickermsg.dump());
            //发送成功之后，将该消息的id放入到队列中，如果一定的时间之后没有收到，则说明该节点已经不可用，需要一处，并且通知所有人
            std::time_t curTime = std::time(nullptr);
            
            key = "ticker_" + to_string(curTimeForCh) + "_" + tit->first;
            fCkTimeMap.insert(std::pair<std::string,std::string>(msgID,key));
            std::cout << ">>>>>>>>>>>>>"<< msgID <<std::endl;
            results.clear();
        }

        if(needNotify) {
            updateRouteInfo();
            needNotify = false;
        }

        //统计下当前节点数量
        std::cout<< "******* current node size: " << manHost->_HostList.size() << "******" <<std::endl;
    }
}
void udpServer::msgThread(){
    //
    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

std::string udpServer::genUuid(){
    random_generator gen;
    uuid id = gen();
    return to_string(id);
}

//更新各自的nodeip信息
void udpServer::updateRouteInfo(){
    std::cout<< "#################### staring update route info "<<std::endl;
    std::vector<std::string> results;
    unsigned short port;
    // json tst,msg;
    for( auto it =  manHost->_HostList.begin();it != manHost->_HostList.end();it++){
        //遍历所有的信息，然后进行更新
        std::cout << "#################### " << it->first << "||" << it->second <<std::endl; 
        boost::algorithm::split(results,it->first,boost::is_any_of("_"));
        
        if(results[0] == _localIp) continue;
 


        port = std::stoi(results[1]);
        json tst(manHost->_HostList);
        json msg = {
            {"msgtype","update"},
            {"data",tst}
        };

        std::cout << "######### notifying this: "
                  << tst.dump()
                  << " to "<< results[0]
                  <<std::endl;
        clientHandler->sendTo(results[0],port,msg.dump());

    }

}