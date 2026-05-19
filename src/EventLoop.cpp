/**
 * @file EventLoop.h
 * @brief EventLoop类的实现
 */

#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/eventfd.h>
#include <errno.h>
#include <memory>

// 线程局部变量，表示当前线程中的EventLoop指针
// 保证一个线程最多只有一个EventLoop对象，每个线程第一次创建EventLoop时设置，该线程后需要创建会触发FATAL
thread_local EventLoop* t_loopInThisThread=nullptr;

// 默认超时时间10秒
const int kPollTimeMs=10000;

int createEventfd() {
    int evtfd=eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd<0) {
        LOG_FATAL("eventfd error:%d\n",errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    :looping_(false)
    ,quit_(false)
    ,callingPendingFunctors_(false)
    ,threadId_(CurrentThread::tid())
    ,poller_(Poller::newDefaultPoller(this))
    ,wakeupFd_(createEventfd())
    ,wakeupChannel_(new Channel(this,wakeupFd_))
{
    LOG_DEBUG("EventLoop create %p in thread %d \n",this,threadId_);
    if (t_loopInThisThread) {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n",t_loopInThisThread,threadId_);
    }else {
        t_loopInThisThread=this;
    }

    // wakeFd_负责唤醒机制，注册读事件
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    close(wakeupFd_);
    t_loopInThisThread=nullptr;
}

void EventLoop::handleRead() {
    uint64_t one=1;
    ssize_t n=read(wakeupFd_,&one,sizeof one);
    if (n!=sizeof one) {
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8 \n",n);
    }
}

void EventLoop::loop() {
    looping_=true;
    quit_=false;

    LOG_INFO("EventLoop %p start looping \n",this);

    while (!quit_) {
        activeChannels_.clear();
        pollReturnTime_=poller_->poll(kPollTimeMs,&activeChannels_);
        for (Channel* channel:activeChannels_) {
            channel->handleEvent(pollReturnTime_);
        }
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n",this);
    looping_=false;
}

void EventLoop::quit() {
    quit_=true;

    if (!isInLoopThread()) {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) {
        cb();
    }else {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}

void EventLoop::wakeup() {
    uint64_t one=1;
    ssize_t n=write(wakeupFd_,&one,sizeof one);
    if (n!=sizeof one) {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n",n);
    }
}

void EventLoop::updateChannel(Channel *channel) {
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel) {
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel) {
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_=true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for (const Functor& functor:functors) {
        functor();
    }
    callingPendingFunctors_=false;
}