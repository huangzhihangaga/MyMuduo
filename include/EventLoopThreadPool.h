/**
 * @file EventLoopThreadPool.h
 * @brief 事件循环线程池类，管理多个EventLoop线程
 */

#ifndef MUDUO_EVENTLOOPTHREADPOOL_H
#define MUDUO_EVENTLOOPTHREADPOOL_H

#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

/**
 * @brief 事件循环线程池，管理多个EventLoop线程，实现多线程的Reactor模型
 * @details 管理多个EventLoopThread(工作线程/SubReactor)
 *          提供轮询负载均衡算法分配EventLoop
 *          支持获取所有EventLoop列表，用于多连接均衡
 *
 *          @par Reactor模型
 *          MainReactor:baseLoop_(主线程),负责accept新连接
 *          SubReactor:工作线程池中的EventLoop，负责已连接socket的I/O
 *
 *          @par 工作流程
 *          1.设置线程池的大小(setThreadNum)
 *          2.启动新线程(start)，创建指定数量的工作线程
 *          3.有新连接时，通过getNextLoop分配一个EventLoop
 *          4.将该连接的Channel注册到分配的EventLoop中
 * @note getNextLoop()和getAllLoops()通常在baseLoop_线程中调用，处理accept，因此不需要额外加锁
 */
class EventLoopThreadPool :noncopyable{
public:
    /// 线程初始化回调函数类型，无返回值，参数为EventLoop对象指针
    /// 通常在EventLoop创建后、调用loop()之前执行，用于初始化
    using ThreadInitCallback=std::function<void(EventLoop*)>;

    /**
     * @brief 构造函数
     * @param baseLoop 主线程的EventLoop(MainReactor)
     * @param nameArg 线程池名称
     */
    EventLoopThreadPool(EventLoop* baseLoop,const std::string& nameArg);

    /**
     * @brief 析构函数
     * @details unique_ptr管理EventLoopThread，会自动释放
     */
    ~EventLoopThreadPool();

    /**
     * @brief 设置工作线程数量
     * @param numThreads 工作线程数量
     * @details 0表示不创建工作线程，所有任务都在baseLoop_中执行
     */
    void setThreadNum(int numThreads){numThreads_=numThreads;}

    /**
     * @brief 启动线程池
     * @param cb 每个工作线程的初始化回调
     * @details 执行流程：
     *          1.设置started_=true
     *          2.创建numThreads_个EventLoopThread
     *          3.启动每个线程，并获取器EventLoop指针
     *          4.如果numThreads_=0且提供了回调，在baseLoop中执行回调
     */
    void start(const ThreadInitCallback& cb=ThreadInitCallback());

    /**
     * @brief 获取下一个EventLoop
     * @return 下一个EventLoop指针
     * @details 轮询策略：
     *          如果有工作线程，轮流返回loops_中的EventLoop
     *          如果没有工作线程，返回baseLoop_
     * @note 使用轮询算法
     */
    EventLoop* getNextLoop();

    /**
     * @brief 获取所有EventLoop
     * @return EventLoop指针的vector
     * @details 如果无工作线程，返回包含baseLoop_的vector
     *          如果有工作线程，返回包含所有工作线程EventLoop的vector
     */
    std::vector<EventLoop*> getAllLoops();

    /**
     * @brief 线程池是否已启动
     * @return true表示已启动
     */
    bool started() const {return started_;}

    /**
     * @brief 获取线程池名称
     * @return 线程池名称
     */
    const std::string name() const {return name_;}

private:
    ///主线程EventLoop(MainReactor)
    EventLoop* baseLoop_;

    /// 线程池名称
    std::string name_;

    /// 是否已启动
    bool started_;

    /// 工作线程数量
    int numThreads_;

    /// 下一个可用的工作线程索引 轮询算法中使用
    int next_;

    /// 工作线程列表对象
    std::vector<std::unique_ptr<EventLoopThread>> threads_;

    /// 工作线程中的EventLoop指针列表，其中的EventLoop都是栈上对象
    std::vector<EventLoop*> loops_;
};

#endif //MUDUO_EVENTLOOPTHREADPOOL_H
