/**
 * @file Callbacks.h
 * @brief TcpConnection相关回调函数类型定义
 */

#ifndef MUDUO_CALLBACKS_H
#define MUDUO_CALLBACKS_H

#include "Timestamp.h"

#include <memory>
#include <functional>

class Buffer;
class TcpConnection;

/**
 * @brief TcpConnection的智能指针类型
 * @details 使用shared_ptr管理TcpConnection，确保在回调时不会失效
 */
using TcpConnectionPtr=std::shared_ptr<TcpConnection>;

/**
 * @brief 连接建立/关闭回调函数类型
 * @param TcpConnectionPtr TcpConnection的智能指针
 * @details 当TCP连接建立成功或关闭时调用
 */
using ConnectionCallback=std::function<void(const TcpConnectionPtr&)>;

/**
 * @brief 连接关闭回调函数类型
 * @param TcpConnectionPtr TcpConnection的智能指针
 * @details 由TcpConnection内部使用，用于通知TcpServer连接已关闭
 * @note 这是库内部使用的回调
 */
using CloseCallback=std::function<void(const TcpConnectionPtr&)>;

/**
 * @brief 写完成回调函数类型
 * @param TcpConnectionPtr TcpConnection的智能指针
 * @details 当输出缓冲区中的数据全部发送完成时调用
 *          适用于需要知道数据已成功发送到对端的场景
 */
using WriteCompleteCallback=std::function<void(const TcpConnectionPtr&)>;

/**
 * @brief 消息接受回调函数类型
 * @param TcpConnectionPtr TcpConnection的智能指针
 * @param Buffer 接收到数据的数据缓冲区
 * @param Timestamp 消息接收时间戳
 * @details socket上收到数据时调用，用户在该调用中解析和处理收到的数据
 */
using MessageCallback=std::function<void(const TcpConnectionPtr&,Buffer*,Timestamp)>;

/**
 * @brief 高水位标记回调函数类型
 * @param TcpConnectionPtr TcpConnection的智能指针
 * @param size_t 输出缓冲区中待发送数据的大小
 * @details 当输出缓冲区大小超过设定的高水位阈值时调用
 *          用户实现流量控制或通知应用层发送速度过快
 */
using HightWaterMarkCallback=std::function<void(const TcpConnectionPtr&,size_t)>;

#endif //MUDUO_CALLBACKS_H
