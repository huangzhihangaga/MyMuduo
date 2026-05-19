/**
 * @file EventLoop.h
 * @brief 事件循环类
 */

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

/**
 * @brief 事件循环类，事件驱动机制
 * @details 运行事件循环，等待并分发IO事件
 *          通过Poller管理Channel的声明周期
 *          提供跨线程任务调用机制 (runInLoop/queueInLoop)
 *          支持通过eventfd唤醒事件循环
 *
 *          @par Reactor模式
 *          EventLoop相当于Reactor角色，不断轮询IO事件
 *          当事件发生时调用对应的Channel回调函数
 *
 *          @par one loop per thread
 *          每个EventLoop绑定一个固定线程，确保线程安全性
 *          通过thread_local变量t_loopInThisThread确保一个线程只有一个EventLoop
 */
class EventLoop :noncopyable{
public:
    /// 无参数、无返回值的回调函数类型
    using Functor=std::function<void()>;

    /**
     * @brief 构造函数
     * @details 如果当前线程已有EventLoop，记录FATAL日志并终止
     * 创建wakeupChannel并注册读事件
     */
    EventLoop();

    /**
     * @brief 析构函数
     * @details 关闭wakeupFd_，清空t_loopInThisThread
     */
    ~EventLoop();

    /**
     * @brief 启动事件循环
     * @details 循环流程
     *          while(!quit_)
     *              1.调用poller_->poll()等待事件
     *              2.遍历activeChannels_，调用channel->handleEvent()
     *              3.执行doPendingFunctors()处理跨线程任务
     * @note 该函数会阻塞在poll调用上，直到quit()被调用
     */
    void loop();

    /**
     * @brief 退出事件循环
     * @details 设置quit_标志
     *          如果调用者不在IO线程中，则通过wakeup()唤醒事件循环，避免阻塞在poll()上
     */
    void quit();

    /**
     * @brief 获取当前轮次poll返回的时间戳
     * @return poll返回的时间戳
     */
    Timestamp pollReturnTime() const {return pollReturnTime_;}

    /**
     * @brief 在IO线程中执行执行回调
     * @param cb 要执行的回调函数
     * @details 如果调用者在IO线程，立即执行
     *          否则将回调加入队列，并唤醒IO线程执行
     */
    void runInLoop(Functor cb);

    /**
     * @brief 将回调加入队列，由IO线程稍后执行
     * @param cb 要执行的回调函数
     * @details 无论调用者是谁，都将回调加入队列
     * 如果当前不在IO线程或正在执行回调，会缓存事件循环
     */
    void queueInLoop(Functor cb);

    /**
     * @brief 唤醒事件循环
     * @details 向wakeup_写入1，触发wakeupChannel的读事件，如果阻塞在poll上即可立刻返回
     */
    void wakeup();

    /**
     * @brief 更新Channel的监听事件
     * @param channel 要更新的Channel
     * @details 调用Poller::updateChannel()
     */
    void updateChannel(Channel* channel);

    /**
     * @brief 移除Channel的监听
     * @param channel 要移除的Channel
     * @details 调用Poller::removeChannel()
     */
    void removeChannel(Channel* channel);

    /**
     * @brief 检查Channel是否被当前EventLoop管理
     * @param channel 要检查的Channel
     * @return true表示被管理
     * @details 调用Poller::hasChannel()
     */
    bool hasChannel(Channel* channel);

    /**
     * @brief 判断当前线程是否是EventLoop所属线程
     * @return true表示是EventLoop所属线程
     */
    bool isInLoopThread() const {return threadId_== CurrentThread::tid();}

private:
    /**
     * @brief 处理wakeupFd_的读文件
     * @detais 读取wakeupFd_中的8个字节
     * 由wakeupChannel_回调触发
     */
    void handleRead();

    /**
     * @brief 执行所有待处理的跨线程回调
     * @details 将pendingFunctors_与一个临时vector交换，减少锁持有事件
     *          在锁外执行所有回调
     *          callingPendingFunctors_避免本轮添加的回调函数在下一轮调用中直到poll超时时间后才执行
     */
    void doPendingFunctors();

    /// Channel列表类型
    using ChannelList=std::vector<Channel*>;

    /// 是否正在循环
    std::atomic_bool looping_;

    /// 是否已请求退出
    std::atomic_bool quit_;

    /// 所属线程id
    const pid_t threadId_;

    /// poll返回的时间戳
    Timestamp pollReturnTime_;

    /// I/O多路复用器
    std::unique_ptr<Poller> poller_;

    /// eventfd文件描述符 用于唤醒事件循环中阻塞的IO线程
    int wakeupFd_;

    /// 管理wakeupFd_
    std::unique_ptr<Channel> wakeupChannel_;

    /// Poller返回的活跃的Channel列表
    ChannelList activeChannels_;

    /// 是否正在执行待处理回调
    std::atomic_bool callingPendingFunctors_;

    /// 待处理的跨线程回调队列
    std::vector<Functor> pendingFunctors_;

    /// 保护pendingFunctors_的互斥锁
    std::mutex mutex_;

};



#endif //MUDUO_EVENTLOOP_H
