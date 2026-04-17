//
// Created by 小晓 on 2026/4/7.
//

#ifndef MUDUO_INETADDRESS_H
#define MUDUO_INETADDRESS_H
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

class InetAddress {
public:
    explicit InetAddress(uint16_t port=0,std::string ip="127.0.0.1");
    explicit InetAddress(const sockaddr_in& addr):addr_(addr){}

    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;

    const sockaddr_in* getSockAddr() const {return &addr_;}
    void setSockAddr(const sockaddr_in& addr){addr_=addr;}
private:
    sockaddr_in addr_;
};


#endif //MUDUO_INETADDRESS_H
