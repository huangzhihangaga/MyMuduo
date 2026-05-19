/**
 * @file InetAddress.cpp
 * @brief InetAddress类的实现
 */
#include "InetAddress.h"
#include "Logger.h"
#include <strings.h>
#include <string>
#include <cstring>
#include <iostream>

InetAddress::InetAddress(uint16_t port, const std::string& ip) {
    bzero(&addr_,sizeof addr_);
    addr_.sin_family=AF_INET;
    addr_.sin_port=htons(port);
    if (inet_pton(AF_INET,ip.c_str(),&addr_.sin_addr)<=0) {
        LOG_ERROR("inet_pton failed ,error=%d",errno);
    }
}

std::string InetAddress::toIp() const {
    char buf[64]={0};
    inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof buf);
    return buf;
}

std::string InetAddress::toIpPort() const {
    char buf[64];
    inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof buf);
    size_t end=strlen(buf);
    uint16_t port=ntohs(addr_.sin_port);
    sprintf(buf+end,":%u",port);
    return buf;
}

uint16_t InetAddress::toPort() const {
    return ntohs(addr_.sin_port);
}