#include "managerhost.hpp"
#include <algorithm>

#include <utility>

//增加新的节点
int managerHost::addNewNode(const std::string & nodeID, const std::string & vhostInfo){

    // json toString = json::parse(vhostInfo);
    // auto k1str = toString["endpoint"][0];

    // auto keyString = k1str["ip"].get<std::string>() + "_" + std::to_string(k1str["port"].get<unsigned short>());
    _HostList.insert(std::pair<std::string,std::string>(nodeID,vhostInfo));

    std::cout << "=======================================" << std::endl;
    for(auto it = _HostList.begin();it != _HostList.end();it++){

        std::cout << "key:"<< it->first << " value:"<< it->second<<std::endl;
    }
    std::cout << "=======================================" << std::endl;
    return 0;
}

//删除节点
int managerHost::deleteNode(hostInfo & rmhi){
    //todo 
    return 1;
}

//修改某一个key的值
int managerHost::changeNode(const std::string & nodeKey,const std::string value){
    auto it = _HostList.find(nodeKey);
    if (it != _HostList.end()){
        it->second = value;
    }
    return 0 ;
}

void managerHost::listNodes(){
    std::cout << "=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=" << std::endl;
    for(auto it = _HostList.begin();it != _HostList.end();it++){

        std::cout << "key:"<< it->first << " value:"<< it->second<<std::endl;
    }
    std::cout << "=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=" << std::endl;
}

std::string managerHost::getPublicNode(std::string nodeID){
    std::cout<<"###############++++++++find the public neighber+++++++++########################" <<std::endl;
    listNodes();
    for(auto it = _HostList.begin();it != _HostList.end();it++){

        auto findPub = json::parse(it->second);
        if(findPub["natmapping"].get<std::string>() == "EndPointIndependent" && 
           findPub["natfiltering"].get<std::string>() == "EndPointIndependent" && 
           nodeID != it->first)
        {
             std::cout << "key:"<< it->first << " value:"<< it->second<<std::endl;
            return it->second;
        }

    }
    return std::string{};
}
