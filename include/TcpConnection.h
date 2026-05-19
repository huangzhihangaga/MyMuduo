/**
 * @file TcpConnection.h
 * @brief TCP连接类
 */

#ifndef MUDUO_TCPCONNECTION_H
#define MUDUO_TCPCONNECTION_H

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <memory>
#include <string>
#include <atomic>
#include <string>

/**
 * @brief TCP连接类，表示一个已建立的TCP连接
 * @details 封装一个已连接的socket文件描述符
 *          管理该连接的Channel和事件回调
 *          提供发送数据和关闭连接的方法
 *          维护输入和输出缓冲区
 *          支持应用层流量控制(高水位标记)
 *
 *          @par 状态机：
 *          kDisconnected -> kConnecting -> kConnected -> kDisconnecting -> kDisconnected
 * @note 继承std::enable_shared_from_this<TcpConnection>，用于在回调中安全延长生命周期
 *       非线程安全，所有操作应该在所属的EventLoop线程中执行
 */
class TcpConnection :noncopyable,public std::enable_shared_from_this<TcpConnection>{
public:
    /**
     * @brief 构造函数
     * @param loop 所属的EventLoop(工作线程)
     * @param nameArg 连接名称
     * @param sockfd 已连接的socket文件描述符
     * @param localAddr 本地地址
     * @param peerAddr 对端地址
     */
    TcpConnection(EventLoop* loop,const std::string& nameArg,int sockfd,
                  const InetAddress& localAddr,const InetAddress& peerAddr);

    /**
     * @brief 析构函数
     */
    ~TcpConnection();

    /**
     * @brief 获取所属的EventLoop
     * @return 所属的EventLoop指针
     */
    EventLoop* getLoop() const {return loop_;}

    /**
     * @brief 获取连接名称
     * @return 连接名称 格式 服务器名称-IP:Port#connId
     */
    const std::string& name() const {return name_;}

    /**
     * @brief 获取本地地址
     * @return 本地地址
     */
    const InetAddress& localAddress() const {return localAddr_;}

    /**
     * @brief 获取对端地址
     * @return 对端地址
     */
    const InetAddress& peerAddress() const {return peerAddr_;}

    /**
     * @brief 是否已连接
     * @return true表示连接已建立
     */
    bool connected() const {return state_ == kConnected;}

    /**
     * @brief 发送数据
     * @param buf 要发送的数据(字符串)
     * @details 线程安全，可以在任意线程中调用
     *          如果在工作线程中调用，则直接调用sendInLoop()发送
     *          如果在其他线程中调用，通过runInLoop()将sendInLoop()放在IO线程中执行     *
     */
    void send(const std::string& buf);

    /**
     * @brief 关闭连接
     * @details 关闭写端，等待对端管壁读端
     */
    void shutdown();

    /**
     * @brief 设置连接建立/关闭回调
     * @param cb 回调函数
     */
    void setConnectionCallback(const ConnectionCallback& cb) {connectionCallback_ = cb;}

    /**
     * @brief 设置消息接收回调
     * @param cb 回调函数
     */
    void setMessageCallback(const MessageCallback& cb) {messageCallback_ = cb;}

    /**
     * @brief 设置写完成回调，数据全部发送到内核缓冲区时调用
     * @param cb 回调函数
     */
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) {writeCompleteCallback_=cb;}

    /**
     * @brief 设置关闭回调
     * @param cb 回调函数
     */
    void setCloseCallback(const CloseCallback& cb) {closeCallback_ = cb;}

    /**
     * @brief 设置高水位回调
     * @param cb 回调函数
     * @param highWaterMark 高水位阈值
     * @details 当输出缓冲区大小超过阈值时调用，用于应用层流量控制
     */
    void setHighWaterMarkCallback(const HightWaterMarkCallback& cb,size_t highWaterMark) {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }

    /**
     * @brief 连接建立完成，在工作线程中调用
     * @details 执行步骤：
     *          1.设置状态为kConnected
     *          2.将Channel与自身进行弱绑定
     *          3.启用读事件监听
     *          4.调用用户连接回调
     */
    void connectEstablished();

    /**
     * @brief 销毁连接，在工作线程中调用
     * @details 执行步骤：
     *          1.如果处于连接状态，设置状态为kDisconnected
     *          2.禁用所有事件
     *          3.调用用户连接回调
     *          4.从Poller中移除Channel
     */
    void connectDestroyed();

private:
    /**
     * @brief 连接状态枚举
     */
    enum StateE {
        kDisconnected, ///< 已断开连接 最初/最终状态
        kConnecting, ///< 正在连接 尚未完全建立
        kConnected, ///< 已连接 正常工作状态
        kDisconnecting ///< 正在断开连接 等待数据发送完成
    };

    /**
     * @brief 设置连接状态
     * @param state 要设置的状态
     */
    void setState(StateE state) {state_ = state;}

    /**
     * @brief 读事件处理，由Channel回调
     * @param receiveTime 事件发生时间
     * @details 从socket读取数据到inputBuffer_，再调用messageCallback_
     */
    void handleRead(Timestamp receiveTime);

    /**
     * @brief 写事件处理，由Channel回调
     * @details 将outputBuffer_中的数据写入socket，如果全部写完，禁用写事件监听
     */
    void handleWrite();

    /**
     * @brief 关闭事件处理，由Channel回调
     * @details 当对端关闭连接或发生错误时调用
     */
    void handleClose();

    /**
     * @brief 错误事件处理，由Channel回调
     * @details 获取socket错误码并记录日志
     */
    void handleError();

    /**
     * @brief 发送数据到内核，在IO线程中执行
     * @param data 数据指针
     * @param len 数据长度
     * @details 实现非阻塞发送和缓冲区管理
     *          如果是第一次写入，直接调用write
     *          如果数据未发送完，存入outputBuffer_
     *          注册读事件，等待socket可写时继续发送
     */
    void sendInLoop(const void* data,size_t len);

    /**
     * @brief 关闭连接，在IO线程中执行
     * @details 如果没有发送数据，立即关闭写端
     */
    void shutdownInLoop();

    /// 所属EventLoop 工作线程
    EventLoop* loop_;

    /// 连接名称
    const std::string name_;

    /// 连接状态
    std::atomic_int state_;

    /// 是否正在读取 (未使用，保留)
    bool reading_;

    /// socket封装
    std::unique_ptr<Socket> socket_;

    /// Channel封装，管理事件
    std::unique_ptr<Channel> channel_;

    /// 本地地址
    const InetAddress localAddr_;

    /// 对端地址
    const InetAddress peerAddr_;

    /// 连接建立/关闭回调
    ConnectionCallback connectionCallback_;

    /// 消息接收回调
    MessageCallback messageCallback_;

    /// 写完成回调
    WriteCompleteCallback writeCompleteCallback_;

    /// 关闭回调，内部使用，为TcpServer::removeConnection()
    CloseCallback closeCallback_;

    /// 高水位回调
    HightWaterMarkCallback highWaterMarkCallback_;

    /// 高水位阈值 (默认64MB)
    size_t highWaterMark_;

    /// 输入缓冲区 (从socket中读入数据)
    Buffer inputBuffer_;

    /// 输出缓冲区 (待发送的数据)
    Buffer outputBuffer_;
};

#endif //MUDUO_TCPCONNECTION_H
