//
// Created by 小晓 on 2026/4/11.
//

#ifndef MUDUO_TCPCONNECTION_H
#define MUDUO_TCPCONNECTION_H

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <memory>
#include <string>
#include <atomic>
#include <string>

class TcpConnection :noncopyable,public std::enable_shared_from_this<TcpConnection>{
public:
    TcpConnection(EventLoop* loop,const std::string& nameArg,int sockfd,
        const InetAddress& localAddr,const InetAddress& peerAddr);
    ~TcpConnection();
    
    EventLoop* getLoop() const {return loop_;}
    const std::string& name() const {return name_;}
    const InetAddress& localAddress() const {return localAddr_;}
    const InetAddress& peerAddress() const {return peerAddr_;}
    
    bool connected() const {return state_ == kConnected;}
    
    void send(const std::string& buf);

    void shutdown();

    void setConnectionCallback(const ConnectionCallback& cb) {connectionCallback_ = cb;}
    void setMessageCallback(const MessageCallback& cb) {messageCallback_ = cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {writeCompleteCallback_=cb;}
    void setCloseCallback(const CloseCallback& cb) {closeCallback_ = cb;}
    void setHighWaterMarkCallback(const HightWaterMarkCallback& cb) {highWaterMarkCallback_ = cb;}

    void connectEstablished();
    void connectDestroyed();

private:
    enum StateE {kDisconnected,kConnecting,kConnected,kDisconnecting};

    void setState(StateE state) {state_ = state;}

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* data,size_t len);

    void shutdownInLoop();

    EventLoop* loop_;
    const std::string name_;
    std::atomic_int state_;
    bool reading_; // 无用

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    CloseCallback closeCallback_;
    HightWaterMarkCallback highWaterMarkCallback_;

    size_t highWaterMark_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;
};



#endif //MUDUO_TCPCONNECTION_H
