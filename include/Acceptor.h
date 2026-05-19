/**
 * @file Acceptor.h
 * @brief 接收器类，负责监听并接受新的TCP连接
 */

#ifndef MUDUO_ACCEPTOR_H
#define MUDUO_ACCEPTOR_H

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class EventLoop;
class InetAddress;

/**
 * @brief TCP连接接受器，封装监听socket的接受连接操作
 * @details 封装监听socket的创建、绑定、监听
 *          使用Channel将监听socket的事件注册到EventLoop
 *          当有新连接到来时，自动调用accept并回调用户函数
 * @note 每个Acceptor只负责一个监听端口，通常一个TcpServer只有一个Acceptor
 */
class Acceptor:noncopyable {
public:
    /// 新连接回调函数类型
    /// sockfd已接受的新连接的socket文件描述符
    /// peerAddr对端地址信息
    /// 当接受新连接成功后调用，用于将新连接交给TcpServer处理
    using NewConnectionCallback=std::function<void(int sockfd,const InetAddress&)>;

    /**
     * @brief 构造函数
     * @param loop 所属的EventLoop(MainReactor)
     * @param listenAddr 监听地址(ip+port)
     * @param reuseport 是否启用SO_REUSEPORT选项
     * @details 执行步骤：
     *          1.创建非阻塞监听socket
     *          2.设置SO_REUSEADDR，开启地址重用
     *          3.设置SO_REUSEPORT,可选端口重用
     *          4.绑定地址
     *          5.给acceptChannel_绑定回调Acceptor::handleRead
     * @note 需要调用listen()才开始监听
     */
    Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport);

    /**
     * @brief 析构函数
     * @details 禁用并移除Channel，但不关闭Socket，Socket的析构函数会自动关闭文件描述符
     */
    ~Acceptor();

    /**
     * @brief 设置新连接回调
     * @param cb 回调函数
     * @details 由TcpServer设置，当新连接到来时调用，在Acceptor::handleRead中accept成功接收后调用
     */
    void setNewConnectionCallback(const NewConnectionCallback& cb) {newConnectionCallback_=cb;}

    /**
     * @brief 表示是否正在监听
     * @return true表示正在监听
     */
    bool listenning() const {return listenning_;}

    /**
     * @brief 开始监听
     * @details 设置listening_为true
     *          调用Socket::listen()开始监听
     *          关注acceptChannel_的读事件
     */
    void listen();

private:
    /**
     * @brief 处理读事件，接受新连接
     * @details 当监听socket可读时调用，说明有新连接到达
     *          执行流程：
     *          1.调用Socket::accept接受新连接
     *          2.如果成功，调用newConnectionCallback_，
     *            如果没有newConnectionCallback_，则无法将socket和地址交给TcpServer，则关闭新连接的文件描述符
     *          3.如果失败，处理错误日志
     * @note 这个函数在EventLoop的IO线程中执行
     */
    void handleRead();

    /// 所属的EventLoop(MainReactor)
    EventLoop* loop_;

    /// 监听socket的封装
    Socket acceptSocket_;

    /// 监听socket的Channel
    Channel acceptChannel_;

    /// 新连接回调，新连接建立后调用
    NewConnectionCallback newConnectionCallback_;

    /// 是否正在监听
    bool listenning_;
};

#endif //MUDUO_ACCEPTOR_H
