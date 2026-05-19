/**
 * @file EventLoopThread.h
 * @brief 事件循环线程类，封装一个包含EventLoop的线程
 */

#ifndef MUDUO_EVENTLOOPTHREAD_H
#define MUDUO_EVENTLOOPTHREAD_H

#include "noncopyable.h"
#include "Thread.h"

#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

class EventLoop;

/**
 * @brief 事件循环线程类，创建一个线程并在其中运行EventLoop
 * @details EventLoopThread将线程和EventLoop绑定在一起，实现了：
 *          自动创建并启动一个线程
 *          在该线程中创建并运行EventLoop
 *          可以用于创建多线程Reactor模型中的工作线程(SubReactor)
 *          每个EventLoopThread管理一个独立的事件循环线程
 */
class EventLoopThread:noncopyable {
public:
    /// 线程初始化回调函数类型，无返回值，参数为EventLoop对象指针
    /// 通常在EventLoop创建后、调用loop()之前执行，用于初始化
    using ThreadInitCallback=std::function<void(EventLoop*)>;

    /**
     * @brief 构造函数
     * @param cb 线程初始化回调，默认为空
     * @param name 线程名称，默认为空
     * @details 初始化成员变量并创建Thread对象，但不会立即启动
     *          线程函数绑定为EventLoopThread::threadFunc
     */
    EventLoopThread(const ThreadInitCallback& cb=ThreadInitCallback(),
                    const std::string& name=std::string());

    /**
     * @brief 析构函数
     * @details 设置existing_=true，如果EventLoop存在则调用quit(),等待线程结束
     */
    ~EventLoopThread();

    /**
     * @brief 启动线程并获取EventLoop指针
     * @return 新线程中创建的EventLoop对象指针
     * @details 执行流程：
     *          1.启动线程(调用thread_.start()时才真正创建线程)
     *          2.等待条件变量，直到新线程完成EventLoop创建
     *          3.返回EventLoop指针
     * @note 使用条件变量确保新线程(执行EventLoopThread::threadFunc())中EventLoop变量初始化后才返回
     */
    EventLoop* startLoop();

private:
    /**
     * @brief 线程入口函数
     * @details 在新线程中执行：
     *          1.在栈上创建EventLoop对象
     *          2.执行初始化回调
     *          3.通知主线程EventLoop已初始化完成
     *          4.启动事件循环，阻塞在loop.loop()
     *          5.事件循环结束后，将loop_设置为nullptr
     */
    void threadFunc();

    /// 指向新线程中创建的EventLoop对象
    EventLoop* loop_;

    /// 是否正在退出
    bool exiting_;

    /// 底层线程对象
    Thread thread_;

    /// 保护loop_的互斥锁
    std::mutex mutex_;

    /// 条件变量，用于同步等待loop_初始化完成
    std::condition_variable cond_;

    /// 线程初始化回调
    ThreadInitCallback callback_;
};



#endif //MUDUO_EVENTLOOPTHREAD_H
