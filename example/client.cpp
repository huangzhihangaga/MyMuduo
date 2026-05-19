/**
 * @file client.cpp
 * @brief Echo客户端 - 基于TcpConnection手动实现
 *
 * 使用方法:
 *   ./echo_client [server_ip] [port]
 *   ./echo_client 127.0.0.1 8888
 *
 * 交互模式: 输入消息后按回车发送，输入 'quit' 退出
 */

#include "EventLoop.h"
#include "InetAddress.h"
#include "Socket.h"
#include "Channel.h"
#include "Buffer.h"
#include "Logger.h"

#include <iostream>
#include <string>
#include <functional>
#include <signal.h>
#include <thread>
#include <atomic>


class EchoClient {
public:
    EchoClient(EventLoop* loop, const InetAddress& serverAddr)
        : loop_(loop)
        , serverAddr_(serverAddr)
        , sockfd_(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0))
        , channel_(loop, sockfd_)
        , connected_(false)
        , running_(true) {

        if (sockfd_ < 0) {
            LOG_FATAL("socket create error\n");
        }

        channel_.setReadCallback(std::bind(&EchoClient::handleRead, this));
        channel_.setWriteCallback(std::bind(&EchoClient::handleWrite, this));
        channel_.setCloseCallback(std::bind(&EchoClient::handleClose, this));
        channel_.setErrorCallback(std::bind(&EchoClient::handleError, this));
    }

    ~EchoClient() {
        if (sockfd_ >= 0) {
            ::close(sockfd_);
        }
    }

    void connect() {
        // 连接服务器
        int ret = ::connect(sockfd_, (struct sockaddr*)serverAddr_.getSockAddr(),
                            sizeof(struct sockaddr_in));

        if (ret < 0) {
            if (errno != EINPROGRESS) {
                LOG_ERROR("connect error: %d\n", errno);
                return;
            }
            // 连接正在进行中，等待写事件
            channel_.enableWriting();
        } else {
            // 连接成功
            onConnected();
        }

        // 注册读事件
        channel_.enableReading();
    }

    void send(const std::string& msg) {
        if (!connected_) {
            std::cout << "Not connected to server!" << std::endl;
            return;
        }

        std::string message = msg;
        if (message.empty() || message.back() != '\n') {
            message += '\n';
        }

        loop_->runInLoop(std::bind(&EchoClient::sendInLoop, this, message));
    }

    void disconnect() {
        running_ = false;
        loop_->runInLoop(std::bind(&EchoClient::disconnectInLoop, this));
    }

    bool isConnected() const { return connected_; }

private:
    void onConnected() {
        connected_ = true;
        struct sockaddr_in local;
        socklen_t len = sizeof(local);
        if (getsockname(sockfd_, (struct sockaddr*)&local, &len) == 0) {
            InetAddress localAddr(local);
            std::cout << "[Client] Connected to server: "
                      << serverAddr_.toIpPort() << std::endl;
            std::cout << "[Client] Local address: "
                      << localAddr.toIpPort() << std::endl;
        }

        // 连接成功后启用读事件
        channel_.enableReading();
    }

    void sendInLoop(const std::string& msg) {
        if (!connected_) {
            return;
        }

        std::cout << "[Client] Sent: " << msg.substr(0, msg.length() - 1) << std::endl;
        outputBuffer_.append(msg.c_str(), msg.size());

        if (!channel_.isWriting()) {
            // 尝试直接写入
            handleWrite();
        }
    }

    void handleRead() {
        int saveErrno = 0;
        ssize_t n = inputBuffer_.readFd(channel_.fd(), &saveErrno);

        if (n > 0) {
            std::string msg = inputBuffer_.retrieveAllAsString();
            // 去除换行符显示
            std::string displayMsg = msg;
            while (!displayMsg.empty() && displayMsg.back() == '\n') {
                displayMsg.pop_back();
            }
            std::cout << "[Server Echo] " << displayMsg << std::endl;
        } else if (n == 0) {
            handleClose();
        } else {
            if (saveErrno != EAGAIN && saveErrno != EWOULDBLOCK) {
                LOG_ERROR("read error: %d\n", saveErrno);
                handleError();
            }
        }
    }

    void handleWrite() {
        if (!connected_) {
            // 处理连接建立时的写事件
            int optval;
            socklen_t optlen = sizeof(optval);
            if (getsockopt(sockfd_, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
                LOG_ERROR("getsockopt error\n");
                return;
            }
            if (optval == 0) {
                onConnected();
                channel_.disableWriting();
            } else {
                LOG_ERROR("connect error: %d\n", optval);
                handleClose();
            }
            return;
        }

        // 正常发送数据
        if (outputBuffer_.readableBytes() > 0) {
            int saveErrno = 0;
            ssize_t n = outputBuffer_.writeFd(channel_.fd(), &saveErrno);

            if (n > 0) {
                outputBuffer_.retrieve(n);
                if (outputBuffer_.readableBytes() == 0) {
                    channel_.disableWriting();
                }
            } else if (n < 0 && saveErrno != EAGAIN && saveErrno != EWOULDBLOCK) {
                LOG_ERROR("write error: %d\n", saveErrno);
                handleClose();
            }
        }
    }

    void handleClose() {
        if (!connected_) {
            return;
        }

        connected_ = false;
        std::cout << "[Client] Disconnected from server" << std::endl;
        channel_.disableAll();
        channel_.remove();
        running_ = false;
    }

    void handleError() {
        std::cout << "[Client] Error occurred" << std::endl;
        handleClose();
    }

    void disconnectInLoop() {
        if (connected_) {
            // 关闭写端
            if (::shutdown(sockfd_, SHUT_WR) < 0) {
                LOG_ERROR("shutdown error\n");
            }
        }
    }

    EventLoop* loop_;
    InetAddress serverAddr_;
    int sockfd_;
    Channel channel_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;
    bool connected_;
    bool running_;
};

static EchoClient* g_client = nullptr;
static EventLoop* g_loop = nullptr;

void signalHandler(int sig) {
    std::cout << "\n[Client] Interrupted, exiting..." << std::endl;
    if (g_client) {
        g_client->disconnect();
    }
    if (g_loop) {
        g_loop->quit();
    }
}

void printHelp() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "      Echo Client" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  <message>     Send message to server" << std::endl;
    std::cout << "  quit / exit   Disconnect and exit" << std::endl;
    std::cout << "  help          Show this help" << std::endl;
    std::cout << "========================================\n" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string serverIp = "127.0.0.1";
    int port = 8888;

    if (argc > 1) {
        serverIp = argv[1];
    }
    if (argc > 2) {
        port = atoi(argv[2]);
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    signal(SIGPIPE, SIG_IGN);

    std::cout << "\n========================================" << std::endl;
    std::cout << "      Echo Client" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Server: " << serverIp << ":" << port << std::endl;
    std::cout << "========================================\n" << std::endl;

    EventLoop loop;
    InetAddress serverAddr(port, serverIp);
    EchoClient client(&loop, serverAddr);

    g_client = &client;
    g_loop = &loop;

    // 连接服务器
    client.connect();

    printHelp();

    // 启动一个线程处理用户输入
    std::atomic<bool> inputRunning(true);
    std::thread inputThread([&]() {
        std::string input;
        while (inputRunning && loop.isInLoopThread() == false) {
            std::getline(std::cin, input);

            if (input == "quit" || input == "exit") {
                std::cout << "[Client] Quitting..." << std::endl;
                client.disconnect();
                loop.quit();
                break;
            } else if (input == "help") {
                printHelp();
            } else if (!input.empty()) {
                client.send(input);
            }
        }
    });

    // 运行事件循环
    loop.loop();

    inputRunning = false;
    inputThread.join();

    std::cout << "[Client] Exited" << std::endl;

    return 0;
}