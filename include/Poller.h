/**
 * @file Poller.h
 * @brief Poller基类，负责I/O多路复用的抽象接口
 */

#ifndef MUDUO_POLLER_H
#define MUDUO_POLLER_H

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

/**
 * @brief I/O多路复用器的抽象基类
 * @details 负责监听文件描述符上的事件，并返回活跃的Channel
 *          通过抽象基类设计，支持多中国I/O多路复用机制
 *          当前只通过EpollPoller提供了epoll模型
 *          使用newDefaultPoller()创建具体实现
 *          管理Channel与文件描述符之间的映射关系(channels_)
 * @note 非线程安全，所有方法都应该在所属的EventLoop线程中调用
 */
class Poller :noncopyable{
public:
    /// 活跃Channel列表类型，用于存储一次poll调用中所有发生事件的Channel指针
    using ChannelList=std::vector<Channel*>;

    /**
     * @brief 构造函数
     * @param loop 所属的EventLoop对象
     */
    Poller(EventLoop* loop);

    /**
     * @brief 虚析构函数，使用默认实现
     * @details 声明为虚函数，确保派生类对象正确析构
     */
    virtual ~Poller()=default;

    /**
     * @brief 查询活跃的IO事件
     * @param timeoutMs 超时时间，毫秒，-1表示无限等待，0表示非阻塞
     * @param activeChannels 输出参数，保存活跃的Channel
     * @return 事件发生的时间戳
     * @details 调用底层IO多路复用函数，如epoll_wait
     *          将发生事件的Channel填到activeChannels_中
     *          纯虚函数，由具体子类实现
     */
    virtual Timestamp poll(int timeoutMs,ChannelList* activeChannels)=0;

    /**
     * @brief 更新Channel的监听事件
     * @param channel 要更新的Channel对象
     * @details 当Channel关心的事件发生变化时调用
     *          在epoll中调用底层epoll_ctl的MOD/ADD操作
     *          纯虚函数，由具体子类实现
     */
    virtual void updateChannel(Channel* channel)=0;

    /**
     * @brief 移除Channel的监听
     * @param channel 要移除的Channel对象
     * @details 当Channel不在需要监听事件时调用
     *          epoll中调用底层的epoll_ctl的DEL操作
     *          纯虚函数，由具体子类实现
     */
    virtual void removeChannel(Channel* channel)=0;

    /**
     * @brief 检查Channel是否被当前Poller管理
     * @param channel 要检查的channel对象指针
     * @return true表示Channel已被当前对象Poller管理
     * @details 通过查找channels_映射表判断
     */
    bool hasChannel(Channel* channel) const;

    /**
     * @brief 创建默认的Poller对象
     * @param loop 所属的EventLoop对象
     * @return Poller对象指针
     */
    static Poller* newDefaultPoller(EventLoop* loop);
protected:
    /// Channel映射表类型，key：文件描述符sockfd   value：对应的Channel指针
    using ChannelMap=std::unordered_map<int,Channel*>;

    /// 存储所有被管理的Channel，用于快速查找文件描述符对应的Channel对象指针
    /// protected修饰，允许派生类访问
    ChannelMap channels_;
private:
    /// 所属的EventLoop对象指针
    EventLoop* ownerLoop_;
};



#endif //MUDUO_POLLER_H
