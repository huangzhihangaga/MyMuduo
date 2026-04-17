//
// Created by 小晓 on 2026/4/11.
//

#ifndef MUDUO_CALLBACKS_H
#define MUDUO_CALLBACKS_H

#include "Timestamp.h"

#include <memory>
#include <functional>


class Buffer;
class TcpConnection;

using TcpConnectionPtr=std::shared_ptr<TcpConnection>;
using ConnectionCallback=std::function<void(const TcpConnectionPtr&)>;
using CloseCallback=std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback=std::function<void(const TcpConnectionPtr&)>;
using MessageCallback=std::function<void(const TcpConnectionPtr&,Buffer*,Timestamp)>;
using HightWaterMarkCallback=std::function<void(const TcpConnectionPtr&,size_t)>;



#endif //MUDUO_CALLBACKS_H
