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


//用于管理host的注册，修改，请求等操作
class managerHost{
    public:
        map<std::string,std::string> _HostList;


        
        managerHost() = default;
        //增加一个新的节点
        int addNewNode(std::string vhostInfo);
        
        //删除节点
        int deleteNode(hostInfo & rmhi);


        map<std::string,std::string> getList();

};