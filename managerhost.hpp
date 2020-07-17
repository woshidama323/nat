#include <string>
#include <map>
#include <sstream>
#include <vector>
#include <iostream>
#include "json.hpp"
#include <boost/lexical_cast.hpp>

/**********************************
#管理所有client过来的ip port nattype
#**********************************/
using json = nlohmann::json;
using namespace std;
struct hostInfo {

    std::string ip;
    int port;
    std::string natType;

    std::string toString(){
        std::ostringstream stringStream;
        stringStream<<ip<<port<<natType;
        std::string getString = stringStream.str();
        return getString;
    }
};

//managerHost用于管理相连节点的数据 增加/删除/修改/查询等基本操作 也包括维护连接节点的信息
class managerHost{
    public:
        map<std::string,std::string> _HostList;
        managerHost() = default;
        //增加一个新的节点
        int addNewNode(const std::string & nodeID, const std::string & vhostInfo);
        
        //删除节点
        int deleteNode(hostInfo & rmhi);

        //改变某一个节点的信息
        int changeNode(const std::string & nodeKey,const std::string value);

        //获取节点list
        void listNodes();

        //后去bootstrap节点
        std::string getPublicNode(std::string nodeID);
};