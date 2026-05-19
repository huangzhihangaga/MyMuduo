/**
 * @file InetAddress.h
 * @brief 网络地址类封装，支持IPv4
 */

#ifndef MUDUO_INETADDRESS_H
#define MUDUO_INETADDRESS_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

/**
 * @brief IPv4网络地址封装类，用于管理一个sockaddr_in
 * @details 封装了sockaddr_in结构体，提供ip和port的便捷操作
 */
class InetAddress {
public:
    /**
     * @brief 通过端口和ip地址构造InetAddress
     * @param port 端口号，默认为0
     * @param ip ip地址字符串，默认为"127.0.0.1"
     */
    explicit InetAddress(uint16_t port=0,const std::string& ip="127.0.0.1");

    /**
     * @brief 通过sockaddr_in构造InetAddress
     * @param addr sockaddr_in结构体
     */
    explicit InetAddress(const sockaddr_in& addr):addr_(addr){}

    /**
     * @brief 获取ip地址字符串
     * @return ip地址的点分十进制字符串，例如"127.0.0.1"
     * @details 使用inet_ntop将ip转为字符串
     */
    std::string toIp() const;

    /**
     * @brief 获取"ip:port"字符串
     * @return "ip:port"字符串
     */
    std::string toIpPort() const;

    /**
     * @brief 获取端口号
     * @return 端口号
     */
    uint16_t toPort() const;

    /**
     * @brief 获取原始 sockaddr_in指针
     * @return sockaddr_in常量指针
     */
    const sockaddr_in* getSockAddr() const {return &addr_;}

    /**
     * @brief 设置sockaddr_in指针
     * @param addr 要设置的sockaddr_in指针
     */
    void setSockAddr(const sockaddr_in& addr){addr_=addr;}
private:
    /// 存储ipv4网络地址信息
    sockaddr_in addr_;
};


#endif //MUDUO_INETADDRESS_H
