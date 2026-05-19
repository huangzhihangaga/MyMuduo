/**
 * @file Channel.h
 * @brief Channel类，负责管理文件描述符的事件监听和回调处理
 */

#ifndef MUDUO_CHANNEL_H
#define MUDUO_CHANNEL_H
#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop;

/**
 * @brief 事件通道类，封装文件描述符及其感兴趣的事件
 * @details 封装了socketfd(fd_)和其感兴趣的事件(events_)，如EPOLLIN、EPOLLOUT事件
 *          保存Poller返回时的实际发生事件(revents_)
 *          绑定事件发生时的回调函数
 *          提供启用/禁用事件的接口
 *          通过tie()方法与TcpConnection对象进行弱绑定，避免在TcpConnection销毁后调用其回调函数
 * @note 每个Channel对象只属于一个EventLoop，且只在该EventLoop所属的线程中运行
 *       体现 one loop per thread 通过保证线程专属，无需加锁
 */
class Channel :noncopyable{
public:
    /// 无参数、无返回值回调函数类型，用于写事件、关闭事件、错误事件回调
    using EventCallback=std::function<void()>;

    /// 接收Timestamp(时间戳)、无返回值回调函数类型，用于读事件回调
    using ReadEventCallback=std::function<void(Timestamp)>;

    /**
     * @brief 构造函数
     * @param loop 所属的EventLoop对象指针
     * @param fd 要管理的文件描述符
     */
    Channel(EventLoop* loop,int fd);

    /**
     * @brief 析构函数
     * @details 不关闭文件描述符，文件描述符通常由上层如Socket管理
     */
    ~Channel();

    /**
     * @brief 处理事件，在Poller返回后调用
     * @param receiveTime 事件发生的时间戳
     * @details 根据tied_标志决定是否需要使用guard
     *          如果tied_=false，直接调用handleEventWithGuard
     *          如果tied_=true，则先尝试提升tie_(weak_ptr)
     *              如果提升成功，说明绑定对象还存活，跳过shared_ptr，调用handleEventWithGuard
     *              如果提升失败，说明对象已销毁，跳过事件处理
     * @note 跳过tie_避免执行回调时外部对象被析构
     */
    void handleEvent(Timestamp receiveTime);

    /**
     * @brief 设置读事件回调
     * @param cb 类型为ReadEventCallback的回调函数
     */
    void setReadCallback(ReadEventCallback cb) { readCallback_=std::move(cb);}

    /**
     * @brief 设置写事件回调
     * @param cb 类型为EventCallback的回调函数
     */
    void setWriteCallback(EventCallback cb) {writeCallback_=std::move(cb);}

    /**
     * @brief 设置关闭事件回调
     * @param cb 类型为EventCallback的回调函数
     */
    void setCloseCallback(EventCallback cb) {closeCallback_=std::move(cb);}

    /**
     * @brief 设置错误事件回调
     * @param cb 类型为EventCallback的回调函数
     */
    void setErrorCallback(EventCallback cb) {errorCallback_=std::move(cb);}

    /**
     * @brief 将Channel与一个对象进行弱绑定
     * @param obj 要绑定的对象，通常是TcpConnection的shared_ptr
     * @details 通过weak_ptr检查对象是否存活
     *          在执行回调时延长绑定对象的生命周期，如果对象已销毁，则跳过回调执行
     */
    void tie(const std::shared_ptr<void>& obj);

    /**
     * @brief 获取文件描述符
     * @return 返回fd_
     */
    int fd() const {return fd_;}

    /**
     * @brief 获取关心的事件
     * @return events_
     */
    int events() const {return events_;}

    /**
     * @brief 设置实际发生的事件，由Poller调用
     * @param revt 实际发生的事件
     */
    void set_revents(int revt) {revents_=revt;}

    /**
     * @brief 启用读事件监听，并调用update()更新Poller
     */
    void enableReading() {events_ |=kReadEvent; update();}

    /**
     * @brief 禁用读事件监听，并调用update()更新Poller
     */
    void disableReading() {events_ &= ~kReadEvent; update();}

    /**
     * @brief 启用写事件监听，并调用update()更新Poller
     */
    void enableWriting() {events_ |= kWriteEvent; update();}

    /**
     * @brief 禁用写事件监听，并调用update()更新Poller
     */
    void disableWriting() {events_ &= ~kWriteEvent; update();}

    /**
     * @brief 禁用所有事件监听，并调用update()更新Poller
     */
    void disableAll() {events_=kNoneEvent; update();}

    /**
     *@brief 是否没有关心任何事件
     * @return true表示没有关系任何事件
     */
    bool isNoneEvent() const {return events_==kNoneEvent;}

    /**
     * @brief 是否关心写事件
     * @return true表示关心写事件
     */
    bool isWriting() const {return events_ & kWriteEvent;}

    /**
     * @brief 是否关心读事件
     * @return true表示关心读事件
     */
    bool isReading() const {return events_ & kReadEvent;}

    /**
     * @brief 获取Channel在Poller中的状态
     * @return kNew 表示Channel未添加到poller中
     *         kAdded 表示Channel已被添加到poller中
     *         kDeleted 表示Channel已中poller中删除，但可能未从底层数据结构中移除
     */
    int state() const {return state_;}

    /**
     * @brief 设置Channel的状态
     * @param state kNew 表示Channel未添加到poller中
     *              kAdded 表示Channel已被添加到poller中
     *              kDeleted 表示Channel已中poller中删除，但可能未从底层数据结构中移除
     */
    void set_state(int state) {state_=state;}

    /**
     * @brief 获取所属的EventLoop
     * @return 所属的EventLoop对象指针
     */
    EventLoop* ownerLoop() {return loop_;}

    /**
     * @brief 从Poller中移除这个Channel
     * @details 调用EventLoop::removeChannel()，最终调用Poller::removeChannel()
     */
    void remove();

private:
    /**
     * @brief 更新Poller中的事件监听
     * @details 调用EventLoop::updateChannel()，最终调用Poller::updateChannel()
     */
    void update();

    /**
     * @brief 实际的事件处理
     * @param receiveTime 事件发生事件
     * @details 实际的事件处理逻辑，被handleEvent()调用
     *          1.EPOLLHUP + 无数据 = 关闭回调
     *          2.EPOLLERR = 错误回调
     *          3.EPOLLIN/EPOLLPRI =读回调
     *          4.EPOLLOUT =写回调
     */
    void handleEventWithGuard(Timestamp receiveTime);

    /// 无事件 0
    static const int kNoneEvent;

    /// 读事件 EPOLLIN | EPOLLPRI
    static const int kReadEvent;

    /// 写事件 EPOLLOUT
    static const int kWriteEvent;

    /// 所属的EventLoop
    EventLoop* loop_;

    /// 管理的文件描述符
    const int fd_;

    /// 关心的事件
    int events_;

    /// 实际发生的事件，由Poller填写
    int revents_;

    /// 表示Channel在Epoll中的生命周期状态
    /// kNew 表示Channel未添加到poller中
    /// kAdded 表示Channel已被添加到poller中
    /// kDeleted 表示Channel已中poller中删除，但可能未从底层数据结构中移除
    int state_;

    /// 弱绑定对象，通常为TcpConnection
    std::weak_ptr<void> tie_;

    /// 是否已进行弱绑定
    bool tied_;

    /// 读事件回调
    ReadEventCallback readCallback_;

    /// 写事件回调
    EventCallback writeCallback_;

    /// 关闭事件回调
    EventCallback closeCallback_;

    /// 错误事件回调
    EventCallback errorCallback_;
};



#endif //MUDUO_CHANNEL_H
