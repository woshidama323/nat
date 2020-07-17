/*
日志实现：头文件的方式

*/
#include <iostream>
#include <ctime>

using namespace std;
class logc{

    public:
        //初始化日志
        logc(string filename){
            if (!filename.empty()){
                //则不存文件，直接屏幕打印
                
            }
        }

        //需要一个模板，这样就不限制类型
        void Debug(string logmsg){
            //如果需要格式化，该如何进行？

            cout<<"=====" <<getCurrentTime()<< "===="<<logmsg<<endl;
        }

        std::string getCurrentTime(){
            std::time_t curTime = std::time(nullptr);
            tm*  t_tm = localtime(&curTime);
            std::string removeReturn = asctime(t_tm);
            removeReturn.pop_back();
            return removeReturn;
        }
};