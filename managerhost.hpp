#include <string>
#include <map>
#include <sstream>
#include <vector>
#include <iostream>
/**********************************
#管理所有client过来的ip port nattype
#**********************************/

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
    private:
        map<std::string,std::vector<struct hostInfo>> _HostList;

    public:
        
        managerHost() = default;
        //增加一个新的节点
        int addNewNode(std::vector<struct hostInfo> vhostInfo);
        
        //删除节点
        int deleteNode(hostInfo & rmhi);


        map<std::string,std::vector<struct hostInfo>> getList();

};