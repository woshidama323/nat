#include <boost/asio.hpp>
#include <iostream>
#include "client.hpp"

client::client(io_service &bindIoServer,unsigned short intPort,std::shared_ptr<ip::udp::socket> _clientSocketPtr)
{
    _clientSocket = _clientSocketPtr;
}

std::string client::LocalIp(std::string & ip){
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
        return addr.to_string();
    } catch (std::exception& e){
        std::cerr << "Could not deal with socket. Exception: " << e.what() << std::endl;
        return std::string{};
    }
    // return ip;
}

int client::sendTo(std::string &remoteIp,unsigned short &remotePort,const std::string & msg){//,std::string &localPort
    try{
        // io_service bindIoServer;
        // udpServer* udpServer = new udpServer;
        // auto hello =  new udpServer();
        // hello->getLocalIP();
        // std::string iptmp = "122.29.3.11";
        // auto localIp = LocalIp(iptmp);
        // // std::cout<<hello->_localIp
        //获取本地ip地址，当前服务的port号
        // std::string msg = std::string("{\"msgtype\":\"detect\",\"data\":{\"localip\":\"why\"}}");
        // json msgj2 = {
        //     {"msgtype","detect"},
        //     {"data",{
        //         {"localip",localIp},
        //         {"localport",localPort}
        //     }}
        // };
        // auto msg = msgj2.dump();
        // auto intPort = boost::lexical_cast<unsigned short>(localPort);
        // auto intRemotePort = boost::lexical_cast<unsigned short>(remotePort);
        // if(std::numeric_limits<unsigned short>::max())
        
        auto remoteEndPoint = boost::asio::ip::udp::endpoint(ip::address().from_string(remoteIp),remotePort);
        boost::system::error_code ignored_error;
        //why need open
        // bindSocket.open(boost::asio::ip::udp::v4());
        std::cout<<"starting respose:"<<remoteIp<<" port:"<<remotePort<<"msg:"<<msg<<std::endl;

        boost::system::error_code errCode;
        // auto testjson = json::parse(msg);
        // std::vector<std::uint8_t> bjsontst = json::to_bson(testjson);
        _clientSocket->send_to(buffer(msg),remoteEndPoint,0,errCode);
        
        std::cout<<"finished the sending,error:"<<errCode<<std::endl;
        return 0;
    }
    catch(const std::exception& e)
    {
        std::cerr << "Got errors: " <<e.what() << '\n';
        return -1;
    }    

}

std::vector<unsigned char> client::encrypto(std::string msg)
{
    // parameters
    // const std::string raw_data = "Hello, plusaes";
    const std::vector<unsigned char> key = plusaes::key_from_string(&"helloharryhannah"); // 16-char = 128-bit
    const unsigned char iv[16] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    };


    // encrypt
    const unsigned long encrypted_size = plusaes::get_padded_encrypted_size(msg.size());
    std::vector<unsigned char> encrypted(encrypted_size);

    plusaes::encrypt_cbc((unsigned char*)msg.data(), msg.size(), &key[0], key.size(), &iv, &encrypted[0], encrypted.size(), true);
    // fb 7b ae 95 d5 0f c5 6f 43 7d 14 6b 6a 29 15 70


    // Hello, plusaes
    return encrypted;
}


// int client::decrypto(std::vector<unsigned char> enmsg){
//     //parameters
//     const std::vector<unsigned char> key = plusaes::key_from_string(&"helloharryhannah"); // 16-char = 128-bit
//     const unsigned char iv[16] = {
//         0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
//         0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
//     };

//     // decrypt
//     unsigned long padded_size = 0;
//     std::vector<unsigned char> enmsg(encrypted_size);

//     plusaes::decrypt_cbc(&enmsg[0], enmsg.size(), &key[0], key.size(), &iv, &enmsg[0], decrypted.size(), &padded_size);
//     return 0;
// }