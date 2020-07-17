#include "node.hpp"
#include "server.hpp"


#define PACKAGENUM 60


Node::Node(io_service& io_service,std::string rip,const unsigned short rport,const unsigned short port,bool publicflag,std::string & localIp):_NodeID{}
,msgQueuePtr{make_shared<std::queue<json>>()}
,fCkTimeMapPtr{make_shared<std::map<std::string,std::string>>()}
,tickerNumPtr{make_shared<std::map<std::string,unsigned short>>()}
,udpServerPtr{make_shared<udpServer>(io_service,rip,rport,port,publicflag,localIp)}
,udpHelpServerPtr{make_shared<udpServer>(io_service,rip,rport,port+1,publicflag,localIp)}
,_count{0}
,_fromCliIp{rip}
,_fromCliPort{rport}
{
    //启动一个timer线程，用于监控是否收到过应答消息
    // std::thread threadFunc(threadhandle);
    //只有public的节点才会启动该线程 将自己放入到publicnode中
    _NodeID = genUuid();

    _masterPort = port;
    //port在+1之前确定是否超过65535
    
    if (_masterPort>=65535){
        _masterPort = 65535;
        _slavePort = _masterPort - 1;
    }else{
        _slavePort = _masterPort + 1;
    }
    

    //初始化managenode;
    manHostPtr = make_shared<managerHost>();

    //放在这里，先发送消息到一个服务器中

    auto getIp = std::string{""};
    _localIp = localIp;//clientHandler->LocalIp(getIp); //std::string{"192.168.65.2"}; //
    _localPort = port;


    if(publicflag){
        _localEpInfo = {
            {"endpoint",{
                {
                    {"ip",_localIp},
                    {"port",_localPort}
                }
            }},
            {"natmapping", "EndPointIndependent"},  //true 表明有mapping 行为
            {"natfiltering","EndPointIndependent"},  //true 表明有filtering行为
        } ;

        json pubNode = {
            {"ip",_localIp},
            {"port",_localPort}
        };
        //将自己加入到public节点群中
        pulicNodes.insert(pubNode.dump());
    }
    threadPtr = make_shared<thread>([this](){
        std::cout<<"starting timer thread"<<std::endl;
        threadhandle();
    });
    threadPtr2 = make_shared<thread>([this](){
        std::cout<<"starting msg thread"<<std::endl;
        msgThread();
    });
}

Node::~Node(){
	printf("Node Destructor: Destroying dispatch threads...\n");

	// Signal to dispatch threads that it's time to wrap up
	// std::unique_lock<std::mutex> lock(lock_);
	// quit_ = true;
	// lock.unlock();
	// cv_.notify_all();

	// // Wait for threads to finish before we exit
	// for(size_t i = 0; i < threads_.size(); i++)
	// {
	// 	if(threads_[i].joinable())
	// 	{
	// 		printf("Destructor: Joining thread %zu until completion\n", i);
	// 		threads_[i].join();
	// 	}
	// }
}

// void Node::start(){}

// void Node::stop(){

// }
std::string Node::getCurrentTime(){
    std::time_t curTime = std::time(nullptr);
    tm*  t_tm = localtime(&curTime);
    std::string removeReturn = asctime(t_tm);
    removeReturn.pop_back();
    return removeReturn;
}

void Node::listMsgQueue()
{
    std::cout << "=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=" << std::endl;
    for(auto it = tickerNumPtr->begin(); it != tickerNumPtr->end();it++ ){
        //list 出来所有time map中的数据
        std::cout << "key:"<< it->first << " value:"<< it->second <<std::endl ;
    }

    std::cout << "------------------------------------------------------" << std::endl;
    std::cout << " count: "<< _count <<" hostlist:"<<manHostPtr->_HostList.size()<<std::endl ;
    //这里增加一个功能，如果当前的list中的size 为空或者1个 那么重新发起探测请求
    if(manHostPtr->_HostList.size()<=1){
        _count++;
    }
    if (_count >= 10){
        json msgj2 = {
            {"nodeID",_NodeID},
            {"msgtype","detect"},
            {"data",{
                {"localip",_localIp},
                {"localport",to_string(_localPort)}
            }}
        };
        auto msg = msgj2.dump();
        udpServerPtr->sendTo(_fromCliIp,_fromCliPort,msg);
        _count = 0;
    }
    std::cout << "=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=" << std::endl;
}

//get public nodes
std::string Node::getPublicNodes(){

    json test;
    for( auto it =  manHostPtr->_HostList.begin();it != manHostPtr->_HostList.end();it++){
        //遍历所有的信息，然后进行更新
        json Item(it->second);
        auto ItemJson = json::parse(Item.get<std::string>());
        if(it->first == _NodeID) continue;

        if (ItemJson["natmapping"].get<std::string>() == "EndPointIndependent" && ItemJson["natfiltering"].get<std::string>() == "EndPointIndependent"){
            // auto localPortStr = std::to_string(_localPort);
            //如果明确是public的则加入到public中用于探测
            return it->second;
            // udpServerPtr->sendTo(IP,Port,msg.dump());
        }
    }
    

}
//更新public节点信息
void Node::updatePublicNode(){
    std::cout<< "#################### staring update public NNNNOde "<<std::endl;
    unsigned short port;
    json tst(manHostPtr->_HostList);
    json msg = {
        {"msgtype","updatepublic"},
        {"nodeID",_NodeID},
        {"data",tst}
    };

    for( auto it =  manHostPtr->_HostList.begin();it != manHostPtr->_HostList.end();it++){
        //遍历所有的信息，然后进行更新
        std::cout << "#################### " << it->first << "||" << it->second <<std::endl; 
        json Item(it->second);
        auto ItemJson = json::parse(Item.get<std::string>());
        if(it->first == _NodeID) continue;

        if (ItemJson["natmapping"].get<std::string>() == "EndPointIndependent" && ItemJson["natfiltering"].get<std::string>() == "EndPointIndependent"){

            auto IP = ItemJson["endpoint"][0]["ip"].get<std::string>();
            auto Port = ItemJson["endpoint"][0]["port"].get<unsigned short>();
            auto pub = IP + "_" + to_string(Port);
            std::cout<< "public type ,start sending connect msg...." <<msg << "to" << pub<<std::endl;
            // auto localPortStr = std::to_string(_localPort);
            //如果明确是public的则加入到public中用于探测
            if (publicFlag){
                pulicNodes.insert(pub);
            }
            std::cout   <<"=="
                        <<getCurrentTime()
                        <<"=="
                        << "######### notifying this: "
                        << tst.dump()
                        << " to "
                        << IP << " "
                        << Port
                        <<std::endl;

            udpServerPtr->sendTo(IP,Port,msg.dump());
        }
    }

}

//更新非public节点信息
void Node::updateNonPublicNode(){
    std::cout<< "#################### staring update Non public NOOOd"<<std::endl;
    // std::vector<std::string> results;
    unsigned short port;
    // json tst,msg;
    json tst(manHostPtr->_HostList);
    json msg = {
        {"msgtype","updatenopublic"},
        {"nodeID",_NodeID},
        {"data",tst}
    };

    for( auto it =  manHostPtr->_HostList.begin();it != manHostPtr->_HostList.end();it++){
        //遍历所有的信息，然后进行更新
        std::cout << "#################### " << it->first << "||" << it->second <<std::endl; 
        json Item(it->second);
        auto ItemJson = json::parse(Item.get<std::string>());
        if(it->first == _NodeID) continue;


        auto IP = ItemJson["endpoint"][0]["ip"].get<std::string>();
        auto Port = ItemJson["endpoint"][0]["port"].get<unsigned short>();
        std::cout <<"=="
                  <<getCurrentTime()
                  <<"=="
                  << "######### notifying this: "
                  << tst.dump()
                  << " to "
                  << IP << " "
                  <<Port
                  <<std::endl;
        udpServerPtr->sendTo(IP,Port,msg.dump());
    }

}

std::string Node::genUuid(){
    random_generator gen;
    uuid id = gen();
    return to_string(id);
}


//用于定时
void Node::threadhandle(){
    std::time_t curTimeForCh;
    std::map<std::string,std::string>::iterator finditkey;
    json findmsg,tickermsg;

    std::vector<std::string> results;
    unsigned short port;
    time_t timePoint;
    bool needNotify = false;

    //用于判断是否在time的队列中找到了这调消息
    bool havefound = false;
    //消息号
    std::string msgID;
    std::string key,checkpoint;
    unsigned short count = 0;
    while(true){
        //sleep 5 s之后做处理
        std::this_thread::sleep_for(std::chrono::seconds(5));
        //检查list中的发送探测的节点id
        //对比当前时间与发送时间的间隔，如果超过预设的值，则踢出，并修改状态，认为不同
        std::cout<<"checking msg res size:"<<fCkTimeMapPtr->size()<<std::endl;
        curTimeForCh = std::time(nullptr);

        //判断nat的类型 //同时兼顾检测是否收到心跳的消息响应 统一一下是，都是等待消息回应，只是收到之后的action不一样而已
        for(auto it = fCkTimeMapPtr->begin();it != fCkTimeMapPtr->end();){
            std::cout<<"=="<<getCurrentTime()<<"=="<<"current checking .... "<< it->first << "--" << it->second <<std::endl;
            //20s timeout 时间，如果没有收到，则认为不再会收到，如果是filtering 的消息回复，则修改响应的值，然后通知，如果是ticker的回复，删除该跳记录，然后通知
            //msgid msgtype_time_ip_port  23gadsff -> filtering_152389_172.0.0.1_60001_nodeid
            boost::algorithm::split(results, it->second, boost::is_any_of("_"));
            timePoint = std::stoi(results[1]);
            auto hostKey = results[4];
            if(curTimeForCh - timePoint > 20){
                //不知道当前是什么类型的消息,但是此时此刻还在map中，说明没有收到回复，做响应的处理
                if(results[0] == "filtering"){
                    finditkey = manHostPtr->_HostList.find(hostKey);
                    if(finditkey !=  manHostPtr->_HostList.end()){
                        //说明有，则更新下数据
                        findmsg = json::parse(finditkey->second);
                        std::cout<< "checking filtering....findmsg"
                                <<findmsg
                                <<findmsg.is_object()
                                <<findmsg.is_string()
                                <<std::endl;
                        findmsg["natfiltering"] = "isFilter";
                        // manHost->_HostList.at(it->first) = findmsg.dump(); // 更新filtering的数据
                        finditkey->second = findmsg.dump();
                    }
                    //删除这一条
                    std::cout<< "erase it..." <<it->first << ".." <<it->second <<std::endl;
                    fCkTimeMapPtr->erase(it++);

                    //通知所有的节点更新数据
                    needNotify = true;

                }
            }else{
                //print map info....
                std::cout<< it->first << ".." <<it->second <<std::endl;
                ++it;
            }

            results.clear();
        }

        for (auto nit = tickerNumPtr->begin();nit != tickerNumPtr->end();){

                if(nit->second >= PACKAGENUM)
                {
                    needNotify = true;
                    for(auto itx = manHostPtr->_HostList.begin();itx != manHostPtr->_HostList.end();){
                        auto itxem = json::parse(itx->second);
                        auto ip = itxem["endpoint"][0]["ip"].get<std::string>();
                        auto port = itxem["endpoint"][0]["port"].get<unsigned short>();
                        auto strkey = "ticker_" + ip + "_" + to_string(port);
                        if(strkey == nit->first){
                            //说明这个节点失效了，删除掉
                            //这样的方法多个条目一次性可以清除掉
                            manHostPtr->_HostList.erase(itx++);
                        }else{
                            itx++;
                        }
                    }

                    //同时也删除该条记录
                    tickerNumPtr->erase(nit++);
                }else{
                    nit++;
                }
        }

        //增加一个节点的事件检查，加入和退出
        for(auto tit = manHostPtr->_HostList.begin();tit != manHostPtr->_HostList.end();tit++){

            //这里需要知道对端的ip 和 端口号
            auto Item = json::parse(tit->second);
            auto Itemobj = Item; // json::parse(Item.get<std::string>());
            std::cout << "@@@@@@@@@@@@@@"
                      << Itemobj << " string_array_object = "
                      << Itemobj.is_string()
                      << Itemobj.is_array()
                      << Itemobj.is_object()
                      << Item << " Item string_array_object = "
                      << Item.is_string()
                      << Item.is_array()
                      << Item.is_object()
                      << std::endl;

            auto IP = Itemobj["endpoint"][0]["ip"].get<std::string>();
            auto Port = Itemobj["endpoint"][0]["port"].get<unsigned short>();
            std::cout<<"=="
                     <<getCurrentTime()
                     <<"=="".........ticker for target: "
                     << IP 
                     << ".."
                     << Port
                     << "\n == localEpInfo"
                     <<_localEpInfo.is_string()
                     <<_localEpInfo.is_array()
                     <<_localEpInfo.is_object()
                     << std::endl;

            tickermsg = {
                {"msgtype","ticker"},
                {"data",_localEpInfo},
                {"msgid",msgID},
                {"nodeID",_NodeID},
            };
            if(_NodeID == tit->first) continue;

            udpServerPtr->sendTo(IP,Port,tickermsg.dump());
            //发送成功之后，将该消息的id放入到队列中，如果一定的时间之后没有收到，则说明该节点已经不可用，需要一处，并且通知所有人
            // std::time_t curTime = std::time(nullptr);

            key = "ticker_" + IP + "_" + to_string(Port);
            auto findticker = tickerNumPtr->find(key);
            if (findticker != tickerNumPtr->end()){
                findticker->second++;
            }else{
                tickerNumPtr->insert(std::pair<std::string,unsigned short>(key,1));
            }

            // std::cout << ">>>>>>>>>>>>>"<< msgID <<std::endl;
            results.clear();
        }

        if(needNotify) {
            updateRouteInfo();
            needNotify = false;
        }

        //统计下当前节点数量
        std::cout<< "******* current node size: " << manHostPtr->_HostList.size() << "******" <<std::endl;
        manHostPtr->listNodes();
        listMsgQueue();
    }
    
}
void Node::msgThread(){

    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if(!msgQueuePtr->empty()){
            auto msg = msgQueuePtr->front();
            MsgProcess(msg);
        }
        
    }
}

void Node::MsgProcess(json recvJson){
        if(recvJson.find("msgtype") != recvJson.end()){
        //说明有这个字段，
        std::string msgType{""},seeNodeID{""},seeRemoteIp{""},localIpStr{""},localPortStr{""},preMsg{""};
        unsigned short seeRemotePort,localPortInt;
        std::vector<std::string> hostInfos;
        recvJson["msgtype"].get_to(msgType);
        recvJson["nodeID"].get_to(seeNodeID);
        recvJson["remoteip"].get_to(seeRemoteIp);
        recvJson["remoteport"].get_to(seeRemotePort);
        recvJson["remoteport"].get_to(seeRemotePort);


        //只有public的节点接受这些请求 请求方只会向公网ip来发送该消息
        if (msgType == "detectfiltering"){
            auto preMsgID = recvJson["msgid"].get<std::string>();
            localIpStr = recvJson["data"]["localip"].get<std::string>();
            localPortStr = recvJson["data"]["localport"].get<std::string>();
            localPortInt = boost::lexical_cast<unsigned short>(localPortStr);

            std::cout<<getCurrentTime()<<"detectfiltering comming:"<< recvJson.dump() <<std::endl;

            //这里不考虑对方与映射的结果，只返回看到的信息
            //1. 主端口返回该请求方detectfilteringres 消息内容 本节点能看到的public的节点信息，还有对方的节点信息 消息事件是对方的消息id
            //2. 调用辅助端口socket 向请求方发该节点相应 detectfilteringres 对方的消息id data可以为空
            //3. 向邻居节点发detectfilterhelp消息，消息内容为该请求方的ip 和 port 
            std::cout<<"=="<<getCurrentTime()<<"=="<< "respose detectfiltering: publicnode and mapping result.." << std::endl;  

            //组装消息 如果复用消息的话很难实现
            auto getPublicNode = manHostPtr->getPublicNode(_NodeID);
            json getNode(getPublicNode);
            auto getNodeJson = json::parse(getNode.get<std::string>());
            
            json mapping ={
                {"ip" , seeRemoteIp},
                {"port", seeRemotePort}
            };
            json nodeinfo = {
                {"ip" , localIpStr},
                {"port", localPortInt}
            };
            json nodeInfoSee = {
                {"natmapping", ""},  //true 表明有mapping 行为
                {"natfiltering",""},  //true 表明有filtering行为
                {"endpoint",{mapping,nodeinfo}}
            };

            json msgCons={
                {getNodeJson},
                {nodeInfoSee}
            };
            
            auto msgID = genUuid();
            json filteringRes = {
                {"msgtype","detectfilterres"},  
                {"msgid",msgID},
                {"nodeID",_NodeID},
                {"data",msgCons},
                {"premsgid",preMsgID}    //premsg 用于显示上一条消息的id
            };

            //开始发送消息 原路返回
            udpServerPtr->sendTo(seeRemoteIp,seeRemotePort,filteringRes.dump());

            //启用辅助端口返回
            json filteringHelpRes = {
                {"msgtype","detectfilterres"},  
                {"msgid",msgID},
                {"nodeID",_NodeID},
                {"data",{}},
                {"premsgid",preMsgID}    //premsg 用于显示上一条消息的id
            };
            udpHelpServerPtr->sendTo(seeRemoteIp,seeRemotePort,filteringHelpRes.dump());

            //发送给邻居协助进行filtering
            auto ip = getNodeJson["endpoint"][0]["ip"].get<std::string>();
            auto port = getNodeJson["endpoint"][0]["port"].get<unsigned short>();
            //组装协助消息
            json filteringHelp = {
                {"msgtype","detectfilterhelp"},  
                {"msgid",msgID},
                {"nodeID",_NodeID},
                {"data",{}},
                {"premsgid",preMsgID}    //premsg 用于显示上一条消息的id
            };

            udpServerPtr->sendTo(ip,port,filteringHelpRes.dump());

        }
        else if (msgType == "detectmapping"){
            //一般节点会同时发送三个包，两个到同一个节点，一个到另外一个节点
            
        }
        else if (msgType == "update"){
            //专用于更新节点信息用
            //如果来源于自己，则直接过掉
            if(seeNodeID == _NodeID){
                return;
            }

            json updateInfo(recvJson["data"]);
            // auto selfEp = _localIp + "_" + to_string(_localPort);
            auto getself = updateInfo.find(_NodeID);
            if (getself != updateInfo.end()){
                //找到自己，将自己的信息放入到本地
                _localEpInfo = json::parse(getself.value().get<std::string>());
            }    
            std::cout<<"=="<<getCurrentTime()<<"=="<<"^^^^^^^^^^^^^^^^^^^update comming"<< updateInfo
                    <<" string_array_object "
                    << updateInfo.is_string()
                    << updateInfo.is_array()
                    << updateInfo.is_object()
                    << "\n_localEpInfo string_array_object: "
                    << _localEpInfo << ".."
                    << _localEpInfo.is_string()
                    << _localEpInfo.is_array()
                    << _localEpInfo.is_object()
                    << "\ngetself.value() string_array_object: "
                    << getself.value() << ".."
                    << getself.value().is_string()
                    << getself.value().is_array()
                    << getself.value().is_object()
                    <<std::endl;
            //update 消息成功收到，然后存到本地 开始发送消息到各个节点，有几个发几个，这里并发的发 逐个发
            //遍历所有的endpoint 除了自己，然后相连
            //更新本地的routing表，然后连接新的节点 
            // manHost->_HostList



            for (auto it = updateInfo.begin();it != updateInfo.end();it++){

                //map 的字符串只能用着样的方式进行转换
                //先转成json 然后获取string 在转成json 真麻烦
                std::cout <<"=="<<getCurrentTime()<<"=="<< "========================update process.........."
                        <<it.key() << "===== " <<it.value()<< std::endl;
                json item(it.value());
                auto itemobj = json::parse(item.get<std::string>());
                // std::cout << "itemobj process.........."
                //           << itemobj
                //           <<itemobj.is_string()
                //           <<itemobj.is_array()
                //           <<itemobj.is_object()
                //           << std::endl;
                auto tarIp = itemobj["endpoint"][0]["ip"].get<string>();
                auto tarPort = itemobj["endpoint"][0]["port"].get<unsigned short>();                          
                auto checkKey = tarIp + "_" +to_string(tarPort);
                if (manHostPtr->_HostList.find(checkKey) != manHostPtr->_HostList.end()){
                    //本地已经有了这个一个目标了，直接跳过
                    std::cout<< "+++++++ does exist in current list: "<<checkKey<<std::endl;
                    continue;
                }

                if (_NodeID == it.key()){
                    manHostPtr->addNewNode(_NodeID,itemobj.dump());
                    continue;
                }

                if(tarIp == seeRemoteIp){
                    manHostPtr->addNewNode(seeNodeID,itemobj.dump());
                    continue;
                }

                //否则与其他的节点相连
                json msg = {
                    {"msgtype","openconnect"},
                    {"nodeID",_NodeID},
                    {"data",_localEpInfo} //这里应该是开始连接，根据不同节点的nat类型开始连接
                };

                //根据对方的类型来决定如何连接
                //1. 是public的
                std::cout<<"@@@@@@@@@@@@@@@@@@@"
                        <<itemobj["natmapping"].get<std::string>()
                        <<itemobj["natfiltering"].get<std::string>()
                        <<std::endl;
                        
                if (itemobj["natmapping"].get<std::string>() == "EndPointIndependent" && itemobj["natfiltering"].get<std::string>() == "EndPointIndependent"){
                    auto pub = tarIp + "_" + to_string(tarPort);
                    std::cout<< "public type ,start sending connect msg...." <<msg << "to" << pub<<std::endl;
                    // auto localPortStr = std::to_string(_localPort);
                    //如果明确是public的则加入到public中用于探测
                    
                    if (publicFlag){
                        pulicNodes.insert(pub);
                    }

                    udpServerPtr->sendTo(tarIp,tarPort,msg.dump());               
                    
                }else {
                    json msgproxy = {
                        {"msgtype","proxyconnect"},
                        {"nodeID",_NodeID},
                        {"data",itemobj} //这里应该是开始连接，根据不同节点的nat类型开始连接
                    };
            
                    std::cout<<"=="<<getCurrentTime()<<"=="<< "start sending proxyconnect msg...." <<msgproxy << "to" << seeRemoteIp << seeRemotePort <<std::endl;

                    //发送update的人不一定是public的server，所以这里不能这里写，得从本地list中是public的节点去寻求帮助
                    for(auto isPubIt = manHostPtr->_HostList.begin();isPubIt != manHostPtr->_HostList.end();isPubIt++){
                        auto body = json::parse(isPubIt->second) ; 
                        if (body["natmapping"].get<std::string>() == "EndPointIndependent" && body["natfiltering"].get<std::string>() == "EndPointIndependent"){
                            seeRemoteIp = body["endpoint"][0]["ip"].get<std::string>();
                            seeRemotePort = body["endpoint"][0]["port"].get<unsigned short>();
                        }
                    }
                    //如果没有，只能发送谁发给我的信息
                    udpServerPtr->sendTo(seeRemoteIp,seeRemotePort,msgproxy.dump());    
                    //如果自己是public的则不需要发
                    auto localJson = _localEpInfo; //json::parse(_localEpInfo.get<std::string>());
                    if (localJson["natmapping"].get<std::string>()== "EndPointIndependent" && localJson["natfiltering"].get<std::string>() == "EndPointIndependent"){
                        //表明自己是public的则直接相连
                        
                    }else{
                        udpServerPtr->sendTo(tarIp,tarPort,msg.dump()); 
                    }                         
                }

            }
        }
        else if (msgType == "proxyconnect")
        {
            std::cout<<"=="<<getCurrentTime()<<"=="<<"holdpunching comming:"<< recvJson["data"] << recvJson["data"].is_array()<<recvJson["data"].is_object()<<recvJson["data"].is_string()<<std::endl;
            auto tarIp = recvJson["data"]["endpoint"][0]["ip"].get<std::string>();
            auto tarPort = recvJson["data"]["endpoint"][0]["port"].get<unsigned short>();
            //有人请求帮忙，解析body中的ip和端口号，然后通知双端
            
            //这是remote端寻求帮忙的，所以这里发给（消息体中的ip）远端的ip和端口
            json remoteinfo = {
                {"ip",seeRemoteIp},
                {"port",seeRemotePort}
            };
            json helpbody = {
                    {"natmapping", recvJson["data"]["natmapping"]},  //true 表明有mapping 行为
                    {"natfiltering",recvJson["data"]["natfiltering"]},  //true 表明有filtering行为
                    {"endpoint",{
                        remoteinfo
                    }}
                };
            json msg = {
                {"msgtype","helpconnect"},
                {"nodeID",_NodeID},
                {"data",helpbody} //这里应该是开始连接，根据不同节点的nat类型开始连接
            };

            udpServerPtr->sendTo(tarIp,tarPort,msg.dump());
            

        }
        else if (msgType == "helpconnect"){
            std::cout<<"help connect is comming:"
                    <<recvJson["data"]
                    <<"string_array_object"
                    <<recvJson["data"].is_string()
                    <<recvJson["data"].is_array()
                    <<recvJson["data"].is_object()
                    <<std::endl;
            auto tarIp = recvJson["data"]["endpoint"][0]["ip"].get<std::string>();
            auto tarPort = recvJson["data"]["endpoint"][0]["port"].get<unsigned short>();

            // //将这个节点加入到我的队列中
            // json nodeInfoSee = {
            //     {"natmapping", "mapping"},  //true 表明有mapping 行为
            //     {"natfiltering",""},  //true 表明有filtering行为
            //     {"endpoint",{mapping,nodeinfo}}
            // };
            // manHost->addNewNode(hostInfos);
            json msg = {
                {"msgtype","openconnect"},
                {"nodeID",_NodeID},
                {"data",_localEpInfo}//{"data",recvJson["data"]} //这里应该是开始连接，根据不同节点的nat类型开始连接
            };

            //尝试发三次 三秒发完
            for (auto i = 0;i<3;i++){
                udpServerPtr->sendTo(tarIp,tarPort,msg.dump());
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            
            // 每个人拿到的都是对端的mapping之后的ip port
            //收到帮助请求之后，收到请求之后，向对端发送连接请求
            //可能是发送的应答也可能是其他，增加一个msg的id号来区分
            

        }
        else if (msgType == "openconnect"){
            std::cout<<"=="<<getCurrentTime()<<"=="
                    <<"-----------------pouching successful "
                    << "ip: "
                    << seeRemoteIp
                    << " port: "
                    << seeRemotePort
                    << "data is :"
                    << recvJson["data"]
                    << recvJson["data"].is_object()
                    << recvJson["data"].is_string()
                    << recvJson["data"].is_array()
                    <<std::endl;
            
            //开始建立心跳，定时发送信息，加入到心跳队列中
            //如果对方是public的则直接相连,
            //则当filtering来处理  收到该请求，返回对方应答
            // auto tarEpStr = seeRemoteIp + "_" + to_string(seeRemotePort) ;
            // tickerNodes.insert(tarEpStr);
            //加入到自己的list中，心跳的来源也是从这里来的
            //当收到connect的连接的时候，要相应，如果相应

            //2020-7-2 有可能，nat下的node 给s1 之后 mapping的ip 与 回s2之后的ip不一样，所以这里需要做区分，不然node与之前的数据不一样
            if (seeRemoteIp != recvJson["data"]["endpoint"][0]["ip"].get<std::string>() || seeRemotePort != recvJson["data"]["endpoint"][0]["port"].get<unsigned short>()){
                auto change = recvJson["data"];
                change["endpoint"][0]["ip"] = seeRemoteIp;
                change["endpoint"][0]["port"] = seeRemotePort;
                std::cout << "new ep is:"
                        << change
                        << " change.string.array.object "
                        << change.is_string()
                        << change.is_array()
                        << change.is_object() << std::endl;

                auto get = manHostPtr->_HostList.find(seeNodeID);
                if (get != manHostPtr->_HostList.end()){


                    get->second = change.dump();
                }else{
                    manHostPtr->addNewNode(seeNodeID,change.dump());
                }
            }else{
                manHostPtr->addNewNode(recvJson["nodeID"].get<std::string>(),recvJson["data"].dump());
            }
            
        }
        else if (msgType == "filteringcheck"){ //走filtering 行为检查
            std::cout<<"=="<<getCurrentTime()<<"=="<<"filteringcheck comming"<<std::endl;

            //需要做一次保护，只能pulicnodes的节点可以发
            localIpStr = recvJson["ip"].get<std::string>();
            localPortInt = recvJson["port"].get<unsigned short>();
            
            json filMsg = {
                {"msgtype","callclient"},
                {"ip",localIpStr},
                {"port",localPortInt},
                {"originip",seeRemoteIp},
                {"originport",seeRemotePort},
                {"msgid",recvJson["msgid"].get<std::string>()},
                {"nodeID",_NodeID},
            };
            udpServerPtr->sendTo(localIpStr,localPortInt,filMsg.dump());

        }
        else if (msgType == "callclient"){
            std::cout<<"=="<<getCurrentTime()<<"=="<<"callclient comming"<<std::endl;
            //只要收到该类型的消息就可以，不用考虑内容。
            //找到除了自己的所有邻居发广播
            json filteringMsgR = {
                        {"msgtype","callclientRes"},
                        {"nodeID",_NodeID},
                        {"ip",seeRemoteIp},
                        {"port",seeRemotePort},
                        {"msgid",recvJson["msgid"].get<std::string>()},
                        {"nodeID",_NodeID},
            };
            std::string nIp;
            unsigned short nPort;

            //知道自己是什么类型了，更新下，同时上报到之前的节点
            auto oIp = recvJson["originip"].get<std::string>();
            auto oPort = recvJson["originport"].get<unsigned short>();

            //这里也发三次
            for(auto i = 0;i<3 ;i++){
                udpServerPtr->sendTo(oIp,oPort,filteringMsgR.dump());
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            
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
            std::cout <<"=="<<getCurrentTime()<<"=="<< "callclientRes is comming" <<std::endl;
            localIpStr = recvJson["ip"].get<std::string>();
            localPortInt = recvJson["port"].get<unsigned short>();
            auto ipKey =  recvJson["nodeID"].get<std::string>();//localIpStr + "_" +  std::to_string(localPortInt);
            //查询本地是否有该节点，如果有
            auto findit = manHostPtr->_HostList.find(ipKey);
            if(findit !=  manHostPtr->_HostList.end()){
                //说明有，则更新下数据
                auto findmsg = json::parse(findit->second);
                findmsg["natfiltering"] = "EndPointIndependent";
                manHostPtr->_HostList.at(ipKey) = findmsg.dump(); // 更新filtering的数据

                //同时踢出timer中的记录
                auto getIt = fCkTimeMapPtr->find(recvJson["msgid"].get<std::string>());
                if (fCkTimeMapPtr->end() != getIt){
                    //说明在20之内收到了回复，则直接删除该记录
                    fCkTimeMapPtr->erase(getIt);
                }
                //此时需要向所有的邻居更新下该信息

            }
            //在经过proxy确认之后的nat的节点，对方是主动连过来的。这个时候更新下
            updateRouteInfo();
        }
        else if (msgType == "ticker"){

            auto data = recvJson["data"];

            std::cout<<getCurrentTime()<<"-from "<<seeRemoteIp<<"xx"<<seeRemotePort<<" is comming xxx "<<data<<".."<<data.is_object()<<std::endl;
            auto msgID = recvJson["msgid"].get<std::string>();
            auto epNodeID = recvJson["nodeID"].get<std::string>();

            if(!data.is_null()){
                auto findNode = manHostPtr->_HostList.find(epNodeID);
                if(findNode != manHostPtr->_HostList.end())
                {
                    findNode->second = data.dump();

                }else {
                    manHostPtr->addNewNode(epNodeID,data.dump());
                }
            }

            json msgRes = {
                {"msgtype","tickerRes"},
                {"msgid",msgID},
                {"nodeID",_NodeID},
            };
            udpServerPtr->sendTo(seeRemoteIp,seeRemotePort,msgRes.dump());

            //因为已经收到ticker 说明 从对方到自己是通的这个时候，之前发的也没有必要一定要等到res的消息回来
            std::string tcheckpoint;
            tcheckpoint = "ticker_" + seeRemoteIp + "_" +to_string(seeRemotePort);
            auto findtt = tickerNumPtr->find(tcheckpoint);
            if(findtt != tickerNumPtr->end()){
                tickerNumPtr->erase(findtt);
            }
        }
        else if (msgType == "tickerRes"){
            //收到ticker相应之后才能确定该节点依然在线
            auto msgID = recvJson["msgid"].get<std::string>();
            std::cout<< "? ??   ?????? ? ?get msgid:" <<msgID <<std::endl;
            std::string checkpoint;


            checkpoint = "ticker_" + seeRemoteIp + "_" +to_string(seeRemotePort);
            auto findt = tickerNumPtr->find(checkpoint);
            if(findt != tickerNumPtr->end()){
                tickerNumPtr->erase(findt);
            }
            //同时将该节点加入到自己的nodelist中，只有连接成功之后的节点可以加入

            
        }else{
            std::cout<<"not support"<<std::endl;
        }
    }
}

//第一次发送探测请求，等待对方回复：主要等对方发过来public的节点信息
void Node::startDetect(){
    auto getip = std::string{""};
    // auto localIpInfo = server.clientHandler->LocalIp(getip);
    auto localIpInfo = _localIp;
    auto msgid = genUuid();

    json endpointobj = {
        {"localip",localIpInfo},
        {"localport",_localPort}
    };

    json dataobj = {
        {"endpoint",{
            {endpointobj}
        }}
    };

    json msgj2 = {
        {"nodeID",_NodeID},
        {"msgtype","detectfiltering"},
        {"data",{
            {dataobj}
        }},
        {"msgid",msgid},
        {"premsgid",""}
    };
    udpServerPtr->sendTo(_fromCliIp,_fromCliPort,msgj2.dump());
}


 void Node::start(){
    //主要让外部调用

 }