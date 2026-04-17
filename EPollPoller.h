//
// Created by 小晓 on 2026/4/8.
//

#ifndef MUDUO_EPOLLPOLLER_H
#define MUDUO_EPOLLPOLLER_H

#include "Poller.h"
#include "Timestamp.h"


#include <vector>
#include <sys/epoll.h>

class Channel;

class EPollPoller : public Poller{
public:
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;

    Timestamp poll(int timeoutMs,ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel *channel) override;

private:
    static const int kInitEventListSize=16;

    void fillActiveChannels(int numEvents,ChannelList* activeChannels) const;
    void update(int operation,Channel* channel);

    using EventList=std::vector<epoll_event>;

    int epollfd_;
    EventList events_;
};



#endif //MUDUO_EPOLLPOLLER_H
