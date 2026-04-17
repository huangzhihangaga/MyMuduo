//
// Created by 小晓 on 2026/4/8.
//
#include "Poller.h"
#include "EPollPoller.h"

#include <stdlib.h>

Poller *Poller::newDefaultPoller(EventLoop *loop) {
    if (::getenv("MUDUO_USE_POLL")) {
        // 生成poll实例
        return nullptr;
    }else {
        return new EPollPoller(loop);
    }
}
