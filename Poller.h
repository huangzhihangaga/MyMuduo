//
// Created by 小晓 on 2026/4/8.
//

#ifndef MUDUO_POLLER_H
#define MUDUO_POLLER_H

#include "noncopyable.h"
#include "Timestamp.h"


#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

class Poller :noncopyable{
public:
    using ChannelList=std::vector<Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller();

    virtual Timestamp poll(int timeoutMs,ChannelList* activeChannels)=0;
    virtual void updateChannel(Channel* channel)=0;
    virtual void removeChannel(Channel* channel)=0;

    bool hasChannel(Channel* channel) const;

    static Poller* newDefaultPoller(EventLoop* loop);
protected:
    // map的key表示sockfd value表示sockfd所属的channel通道类型
    using ChannelMap=std::unordered_map<int,Channel*>;
    ChannelMap channels_;
private:
    EventLoop* ownerLoop_;
};



#endif //MUDUO_POLLER_H
