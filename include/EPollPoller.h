/**
 * @file EPollPoller.h
 * @brief Epoll IO多路复用的具体实现
 */

#ifndef MUDUO_EPOLLPOLLER_H
#define MUDUO_EPOLLPOLLER_H

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class Channel;

/**
 * @brief 基于epoll的IO多路复用器实现
 * @details EPollPoller是Poller就的派生类
 */
class EPollPoller : public Poller{
public:
    /**
     * @brief 构造函数
     * @param loop 所属的EventLoop对象
     * @details 执行流程：
     *          1.调用极了Poller的构造函数
     *          2.调用epoll_create1(EPOLL_CLOEXEC)创建epoll实例
     *          3。初始化events_，大小为kInitEventListSize
     * @note 如果epoll_create1失败，记录FATAL日志并终止程序
     */
    EPollPoller(EventLoop* loop);

    /**
     * @brief 析构函数
     * @details 关闭epollfd_
     */
    ~EPollPoller() override;

    /**
     * @brief 等待监听的文件描述符上发生感兴趣的事件
     * @param timeoutMs 超时时间(毫秒)，-1 表示无限等待，0 表示非阻塞
     * @param activeChannels 输出参数，存储发生事件的Channel列表
     * @return 事件发生的时间戳
     * @details 实现流程：
     *          1.调用epoll_wait等待事件
     *          2.如果有事件，调用fillActiveChannels填充活跃Channel
     *          3.如果事件数组已满，自动扩容
     *          4.处理超时和错误情况(EINTR可接受，其他错误记录日志)
     */
    Timestamp poll(int timeoutMs,ChannelList* activeChannels) override;

    /**
     * @brief 更新Channel的事件监听
     * @param channel 要更新的Channel对象
     * @details 根据Channel的状态执行不同操作
     *          kNew/kDeleted 调用EPOLL_CTL_ADD添加
     *          kAdded 且 无事件 调用EPOLL_CTL_DEL删除
     *          kAdded 且 有时间 调用EPOLL_CTL_MOD修改
     */
    void updateChannel(Channel* channel) override;

    /**
     * @brief 移除Channel的事件监听
     * @param channel 要移除的Channel对象
     * @details 执行流程：
     *          1.从channels_映射表中删除
     *          2.如果Channel状态为kAdded，调用EPOLL_CTL_DEL
     *          3.将Channel状态重置为kNew
     */
    void removeChannel(Channel *channel) override;

private:
    /// epoll_event事件列表的初始大小 16
    static const int kInitEventListSize=16;

    /**
     * @brief 填充活跃的Channel事件列表
     * @param numEvents 发生事件的文件描述符数量
     * @param activeChannels 输出参数，存储活跃Channel
     * @details 遍历events_数组的前numEvents个元素
     *          将每个epoll_event中的ptr转换为Channel指针
     *          并设置revents后加入活跃列表
     */
    void fillActiveChannels(int numEvents,ChannelList* activeChannels) const;

    /**
     * @brief 封装epoll_ctl系统调用
     * @param operation 操作类型 EPOLL_CTL_ADD/MOD/DEL
     * @param channel 目标Channel
     * @details 将Channel关心的事件转换为epoll_event结构，并调用epoll_ctl执行操作
     */
    void update(int operation,Channel* channel);

    /// epoll_event列表类型
    using EventList=std::vector<epoll_event>;

    /// epoll文件描述符
    int epollfd_;

    /// 存储epoll_wait返回的事件数组
    EventList events_;
};



#endif //MUDUO_EPOLLPOLLER_H
