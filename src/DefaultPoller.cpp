/**
 * @file DefaultPoller.cpp
 * @brief Poller工厂方法的实现，根据环境变量选择具体的IO多路复用实现
 */

#include "Poller.h"
#include "EPollPoller.h"

#include <stdlib.h>

/**
 * @brief 创建默认的Poller对象
 * @param loop 所属的EventLoop对象指针
 * @return Poller对象指针，当前仅返回EpollPoller对象
 * @details 当前仅支持返回EpollPoller实例
 */
Poller *Poller::newDefaultPoller(EventLoop *loop) {
    if (::getenv("MUDUO_USE_POLL")) {
        // 如果设置了环境变量"MUDUO_USE_POLL"，返回nullptr
        return nullptr;
    }else {
        return new EPollPoller(loop);
    }
}
