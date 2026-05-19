/**
 * @file Channel.cpp
 * @brief Channel类的实现
 */

#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent=0;
const int Channel::kReadEvent=EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent=EPOLLOUT;


Channel::Channel(EventLoop *loop, int fd)
    :loop_(loop)
    ,fd_(fd)
    ,events_(0)
    ,revents_(0)
    ,state_(-1)
    ,tied_(false){
    
}

Channel::~Channel() {
    
}

void Channel::tie(const std::shared_ptr<void> & obj) {
    tie_=obj;
    tied_=true;
}

void Channel::update() {
    loop_->updateChannel(this);
}

void Channel::remove() {
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime) {
    if (tied_) {
        std::shared_ptr<void> guard=tie_.lock();
        if (guard) {
            handleEventWithGuard(receiveTime);
        }
        // 如果guard为空，说明绑定对象已销毁，直接跳过
    }else {
        // 如果没有绑定对象，则直接执行
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime) {
    LOG_INFO("channel handleEvent revents:%d\n",revents_);

    // 关闭事件处理
    // EPOLLHUP 连接挂起
    // 只有当没有EPOLLIN时才触发关闭回调
    // 如果有EPOLLIN，说明还有数据可读，应先读数据在关闭
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (closeCallback_) {
            closeCallback_();
        }
    }

    // 错误事件处理
    if (revents_ & EPOLLERR) {
        if (errorCallback_) {
            errorCallback_();
        }
    }

    // 读事件处理
    if (revents_ & (EPOLLIN | EPOLLPRI)) {
        if (readCallback_) {
            readCallback_(receiveTime);
        }
    }

    // 写事件处理
    if (revents_ & EPOLLOUT) {
        if (writeCallback_) {
            writeCallback_();
        }
    }
}
