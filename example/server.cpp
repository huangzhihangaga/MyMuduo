/**
 * @file server.cpp
 * @brief Echo服务器 - 基于TcpServer
 *
 * 使用方法: ./echo_server [port] [thread_num]
 */

#include "EventLoop.h"
#include "TcpServer.h"
#include "InetAddress.h"
#include "Buffer.h"
#include "Timestamp.h"
#include "Logger.h"

#include <iostream>
#include <string>
#include <signal.h>


static TcpServer* g_server = nullptr;

void signalHandler(int sig) {
    std::cout << "\nReceived signal " << sig << ", shutting down..." << std::endl;
    if (g_server) {
        // 优雅退出
    }
    exit(0);
}

void onConnection(const TcpConnectionPtr& conn) {
    if (conn->connected()) {
        std::cout << "[Server] New connection from: "
                  << conn->peerAddress().toIpPort() << std::endl;
        LOG_INFO("New connection from: %s\n", conn->peerAddress().toIpPort().c_str());
    } else {
        std::cout << "[Server] Connection closed: "
                  << conn->peerAddress().toIpPort() << std::endl;
        LOG_INFO("Connection closed: %s\n", conn->peerAddress().toIpPort().c_str());
    }
}

void onMessage(const TcpConnectionPtr& conn, Buffer* buffer, Timestamp receiveTime) {
    std::string msg = buffer->retrieveAllAsString();

    if (msg.empty()) {
        return;
    }

    // 去除末尾换行符（用于显示）
    std::string displayMsg = msg;
    while (!displayMsg.empty() && displayMsg.back() == '\n') {
        displayMsg.pop_back();
    }

    std::cout << "[Server] Received " << msg.size() << " bytes from "
              << conn->peerAddress().toIpPort() << ": \""
              << displayMsg << "\"" << std::endl;

    // Echo: 原样返回
    conn->send(msg);

    std::cout << "[Server] Echoed back to "
              << conn->peerAddress().toIpPort() << std::endl;
}

int main(int argc, char* argv[]) {
    int port = 8888;
    int threadNum = 0;

    if (argc > 1) {
        port = atoi(argv[1]);
    }
    if (argc > 2) {
        threadNum = atoi(argv[2]);
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGPIPE, SIG_IGN);

    std::cout << "\n========================================" << std::endl;
    std::cout << "      Echo Server Started" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Listening on:   0.0.0.0:" << port << std::endl;
    std::cout << "Worker threads: " << (threadNum == 0 ? "Main thread only" : std::to_string(threadNum)) << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << "========================================\n" << std::endl;

    EventLoop loop;
    InetAddress listenAddr(port);
    TcpServer server(&loop, listenAddr, "EchoServer");

    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);

    if (threadNum > 0) {
        server.setThreadNum(threadNum);
    }

    g_server = &server;
    server.start();

    loop.loop();

    return 0;
}