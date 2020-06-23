#include "managerhost.hpp"
#include <algorithm>

#include <utility>

//增加新的节点
int managerHost::addNewNode(std::vector<struct hostInfo> vhostInfo){
    if (vhostInfo.size() == 0){
        return 0;
    }
    // auto kstring = std::hash<std::string>{}(vhostInfo.front().toString());
    _HostList.insert(std::pair<std::string,std::vector<struct hostInfo>>(vhostInfo.front().toString(),vhostInfo));

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

map<std::string,std::vector<struct hostInfo>>  managerHost::getList(){
    return _HostList;
}