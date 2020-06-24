#include "managerhost.hpp"
#include <algorithm>

#include <utility>

//增加新的节点
int managerHost::addNewNode(std::string vhostInfo){
    if (vhostInfo.length() == 0){
        return 0;
    }
    // auto kstring = std::hash<std::string>{}(vhostInfo.front().toString());
    json toString(vhostInfo);
    auto k1str = json::parse(toString[0].get<std::string>());

    // std::cout<<"+++++"<<k1str.is_object()<<std::endl;
    // return 0;

    auto keyString = k1str["ip"].get<std::string>() + std::to_string(k1str["port"].get<unsigned short>());
    _HostList.insert(std::pair<std::string,std::string>(keyString,vhostInfo));

    for(auto it = _HostList.begin();it != _HostList.end();it++){

        std::cout<< it->first <<std::endl;
    }
    return 0;
}

//删除节点
int managerHost::deleteNode(hostInfo & rmhi){

    // auto isExist = find(_HostList.begin(),_HostList.end(),rmhi);
    // if (isExist != _HostList.end()){
    //     //说明里面有，直接删除
    //     _HostList.erase(isExist);
    //     return 0;
    // }
    
    //没有找到 不需要删除
    return 1;
}

map<std::string,std::string>  managerHost::getList(){
    return _HostList;
}