//
// Created by 小晓 on 2026/4/7.
//

#ifndef MUDUO_EVENTLOOP_H
#define MUDUO_EVENTLOOP_H
#include <functional>
#include <vector>
#include <atomic>
#include <unistd.h>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

class EventLoop :noncopyable{
public:
    using Functor=std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    Timestamp pollReturnTime() const {return pollReturnTime_;}

    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);

    void wakeup();

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    bool isInLoopThread() const {return threadId_== CurrentThread::tid();}

private:
    void handleRead();
    void doPendingFunctors();

    using ChannelList=std::vector<Channel*>;

    std::atomic_bool looping_;
    std::atomic_bool quit_;
    const pid_t threadId_;
    Timestamp pollReturnTime_;
    std::unique_ptr<Poller> poller_;

    int wakeupFd_; // 通过该成员唤醒subloop
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;

    std::atomic_bool callingPendingFunctors_; // 表示当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_;
    std::mutex mutex_;

};



#endif //MUDUO_EVENTLOOP_H
