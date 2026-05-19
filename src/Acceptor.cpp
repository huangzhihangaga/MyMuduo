/**
 * @file Acceptor.cpp
 * @brief Acceptor类的实现
 */

#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

/**
 * @internal
 * @brief 创建非阻塞的socket
 * @return socket文件描述符
 * @details 使用socket()系统调用创建socket，设置
 *          SOCK_NONBLOCK:非阻塞模式
 *          SOCK_CLOEXXEC:防止fork后文件描述符泄漏
 */
static int createNonblocking() {
    int sockfd=::socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,0);
    if (sockfd<0) {
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n",__FILE__,__FUNCTION__,__LINE__,errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    :loop_(loop)
    ,acceptSocket_(createNonblocking())
    ,acceptChannel_(loop_,acceptSocket_.fd())
    ,listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(reuseport);
    acceptSocket_.bindAddress(listenAddr);

    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
}

Acceptor::~Acceptor() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen() {
    listenning_=true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead() {
    InetAddress peerAddr;
    int connfd=acceptSocket_.accept(&peerAddr);
    if (connfd>=0) {
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd,peerAddr);
        }else {
            close(connfd);
        }
    }else {
        LOG_ERROR("%s:%s:%d accept err:%d \n",__FILE__,__FUNCTION__,__LINE__,errno);
        if (errno==EMFILE) {
            LOG_ERROR("%s:%s:%d sockfd reach limit! \n",__FILE__,__FUNCTION__,__LINE__);
        }
    }
}
