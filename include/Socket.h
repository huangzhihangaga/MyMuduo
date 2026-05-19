/**
 * @file Socket.h
 * @brief Socket封装类，提供TCP socket的基础操作
 */

#ifndef MUDUO_SOCKET_H
#define MUDUO_SOCKET_H

#include "noncopyable.h"

class InetAddress;

/**
 * @brief Socket封装类，用于管理一个socket文件描述符
 * @details 封装了TCP socket常用的操作，包括绑定、监听、接受连接、
 * 关闭写端以及各种socket选项的设置
 * 采用RAII机制，在析构中自动关闭文件描述符
 * @note 该类不可拷贝
 */
class Socket :noncopyable{
public:
    /**
     * @brief 构造函数，接管已创建的socket文件描述符
     * @param sockfd 已创建的文件描述符
     * @details explicit防止隐式类型转换
     */
    explicit Socket(int sockfd):sockfd_(sockfd){}

    /**
     * @brief 析构函数，在析构时关闭文件描述符
     */
    ~Socket();

    /**
     * @brief 获取socket文件描述符
     * @return 管理的文件描述符的值
     */
    int fd() const {return sockfd_;}

    /**
     * @brief 绑定地址到socket
     * @param localaddr 本地地址 ip+port
     * @details 绑定失败会输出FATAL日志并终止程序
     */
    void bindAddress(const InetAddress& localaddr);

    /**
     * @brief 开始监听连接请求
     * @details 默认监听队列大小为1024，监听失败会输出FATAL日志并终止程序
     */
    void listen();

    /**
     * @brief 接受一个新连接
     * @param peeraddr 输出参数，用于存储对端地址信息
     * @return 新连接的socket文件描述符，失败返回-1
     * @details 使用accept4()，相比accept()可以原子的设置标志位
     *          SOCK_NONBLOCK 新的socket设置为非阻塞
     *          SOCK_CLOEXEC 当执行exec系列函数时自动关闭这个文件描述符
     */
    int accept(InetAddress* peeraddr);

    /**
     * @brief 关闭写端
     * @details 调用shutdown(SHUT_WR)关闭socket的写端
     * 本端不再发送数据，但是仍可接收数
     */
    void shutdownWrite();

    /**
     * @brief 设置TCP_NODELAY选项，禁用Nagle算法
     * @param on true表示启用TCP_NODELAY,禁用Nagle，false表示关闭
     */
    void setTcpNoDelay(bool on);

    /**
     * @brief 设置SO_REUSEADDR选项，地址重用
     * @param on true表示启动地址重用，false表示关闭
     */
    void setReuseAddr(bool on);

    /**
     * @brief 设置SO_REUSEPORT选项，端口重构用
     * @param on true表示启用端口重用，false表示关闭
     */
    void setReusePort(bool on);

    /**
     * @brief 设置SO_KEEPALIVE选项，保持连接活跃
     * @param on true表示启用TCP Keep-Alive，false表示关闭
     */
    void setKeepAlive(bool on);
private:
    /// 管理的socket文件描述符，初始化后不可修改
    const int sockfd_;
};



#endif //MUDUO_SOCKET_H
