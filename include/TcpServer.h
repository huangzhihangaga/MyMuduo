/**
 * @file TcpServer.h
 * @brief TCP服务器类
 */

#ifndef MUDUO_TCPSERVER_H
#define MUDUO_TCPSERVER_H

#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "TcpConnection.h"

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

/**
 * @brief TCP服务器类，管理所有TCP连接的生命周期
 * @details 封装：
 *          Acceptor：负责监听新连接
 *          EventLoopThreadPool：管理IO线程池
 *          connections_活跃的TcpConnection的映射表
 *
 *          @par 多线程模型 one loop per thread
 *          MainReactor:用户传入的mainLoop，负责accept新连接
 *          SubReactor:线程池中的工作线程，负责已连接socket的IO
 *
 *          @par 使用流程
 *          1.创建EventLoop
 *          2.创建TcpServer，传入地址和端口
 *          3。设置连接、消息、写完成等回调
 *          4.start()启动服务器
 *          5.loop.loop()启动事件循环
 */
class TcpServer :noncopyable{
public:
    /**
     * @brief 线程初始化回调函数类型
     * @param EventLoop 工作线程的EventLoop
     * @details 每个工作线程启动是调用，用于线程初始化
     */
    using ThreadInitCallback=std::function<void(EventLoop*)>;

    /**
     * @brief 端口重用选项枚举
     */
    enum Option {
        kNoReusePort, ///< 不启用SO_REUSEPORT
        kReusePort,///< 启用SO_REUSEPORT
    };

    /**
     * @brief 构造函数
     * @param loop 主线程的EventLoop(MainReactor)
     * @param listenAddr 监听地址
     * @param nameArg 服务器名称
     * @param option 端口重用选项，默认为kNoReusePort
     * @details 执行流程：
     *          1.检查传入的loop是否为nullptr
     *          2.创建Acceptor
     *          3.创建EventLoopThreadPool(IO工作线程池)
     *          4.设置Acceptor的新连接回调
     * @note 传入的loop不能为空
     */
    TcpServer(EventLoop* loop,const InetAddress& listenAddr,const std::string& nameArg,Option option=kNoReusePort);

    /**
     * @brief 析构函数
     * @details 清理所有TcpConnection，保证每个连接正确销毁
     *          1.遍历所有connection_映射表
     *          2.重置shared_ptr减少应用次数
     *          3.安排每个连接在所属线程中销毁
     * @note 在一轮循环中，conn先使得引用计数加一
     *       reset()使得引用计数减一
     *       让conn所属线程执行TcpConnection::connectDestroyed()
     *       当一轮for循环结束，conn退出生命周期，shared_ptr引用技术再次减一
     *       当该连接上的回调函数执行完成后，shared_ptr引用技术再次减一
     *       此时TcpConnection才真正释放
     */
    ~TcpServer();

    /**
     * @brief 设置线程初始化回调
     * @param cb 回调函数
     * @details 在工作线程启动是执行
     */
    void setThreadInitCallback(const ThreadInitCallback& cb) {threadInitCallback_=cb;}

    /**
     * @brief 设置连接建立/关闭回调
     * @param cb 回调函数
     */
    void setConnectionCallback(const ConnectionCallback& cb) {connectionCallback_=cb;}

    /**
     * @brief 设置消息接收回调
     * @param cb 回调函数
     */
    void setMessageCallback(const MessageCallback& cb){messageCallback_=cb;}

    /**
     * @brief 设置写完成回调
     * @param cb 回调函数
     */
    void setWriteCompleteCallback(const WriteCompleteCallback& cb){writeCompleteCallback_=cb;}

    /**
     * @brief 设置IO线程数量
     * @param numThreads 线程数量，0表示所有IO都在mainLoop中执行
     * @note 必须在start()前调用
     */
    void setThreadNum(int numThreads);

    /**
     * @brief 启动服务器
     * @details 执行流程：
     *          1.启动线程池，创建工作线程
     *          2.在mainLoop中执行Acceptor::listen()，开始监听
     */
    void start();

private:
    /**
     * @brief 处理新连接，由Acceptor回调
     * @param sockfd 新连接的socket文件描述符
     * @param peerAddr 对端地址
     * @details 在Acceptor::handle接收到连接后执行
     *          执行流程：
     *          1.从线程池中获取一个EventLoop(轮询)
     *          2.生成唯一的连接名称，格式 name_-IP:Port#connId
     *          3.获取本地地址(getsockname)
     *          4.创建TcpConnection对象
     *          5.保存到connections_映射表
     *          6.设置各种回调
     *          7.在工作线程中调用TcpConnection::connectEstablish()
     */
    void newConnection(int sockfd,const InetAddress& peerAddr);

    /**
     * @brief 移除连接，由TcpConnection的关闭回调触发
     * @param conn 要移除的连接
     * @details 将实际的的清理操作放到mainLoop中执行，保证线程安全
     */
    void removeConnection(const TcpConnectionPtr& conn);

    /**
     * @brief 在mainLoop中实际执行连接移除
     * @param conn 要移除的连接
     * @details 从connections_中删除
     * 并在连接所属的EventLoop中执行TcpConnection::connectDestroyed()
     */
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    /// 连接名称和连接的智能指针的映射表
    using ConnectionMap=std::unordered_map<std::string,TcpConnectionPtr>;

    /// 主线程的EventLoop(mainLoop/MainReactor)
    EventLoop* loop_;

    /// IP:Port格式字符串
    const std::string ipPort_;

    /// 服务器名称
    const std::string name_;

    /// 连接接受器，运行在mainloop
    std::unique_ptr<Acceptor> acceptor_;

    /// IO线程池(one loop per thread)
    std::shared_ptr<EventLoopThreadPool> threadPool_;

    /// 连接建立/关闭回调
    ConnectionCallback connectionCallback_;

    /// 消息接受回调
    MessageCallback messageCallback_;

    /// 写完成回调
    WriteCompleteCallback writeCompleteCallback_;

    /// 线程初始化回调
    ThreadInitCallback threadInitCallback_;

    /// 启动状态计数器，保证只启动一次
    std::atomic_int started_;

    /// 下一个连接的id，用于生成唯一名称
    int nextConnId_;

    /// 连接映射表 名称:TcpConnectionPtr
    ConnectionMap connections_;
};



#endif //MUDUO_TCPSERVER_H
