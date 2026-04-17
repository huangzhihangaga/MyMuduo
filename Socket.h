//
// Created by 小晓 on 2026/4/10.
//

#ifndef MUDUO_SOCKET_H
#define MUDUO_SOCKET_H

#include "noncopyable.h"

class InetAddress;

class Socket :noncopyable{
public:
    explicit Socket(int sockfd):sockfd_(sockfd){}
    ~Socket();

    int fd() const {return sockfd_;}
    void bindAddress(const InetAddress& localaddr);
    void listen();
    int accept(InetAddress* peeraddr);

    void shutdownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);
private:
    const int sockfd_;
};



#endif //MUDUO_SOCKET_H
