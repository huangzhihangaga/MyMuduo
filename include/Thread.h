/**
 * @file Thread.h
 * @brief 线程封装类
 */

#ifndef MUDUO_THREAD_H
#define MUDUO_THREAD_H

#include "noncopyable.h"

#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <string>
#include <atomic>

/**
 * @brief 线程封装类，提供线程的创建、启动和等待功能
 * @details 不可拷贝
 */
class Thread :noncopyable{
public:
    /// 线程函数类型定义，无参数无返回值，可以是普通函数、lambda、std::bind结果
    using ThreadFunc=std::function<void()>;

    /**
     * @brief 构造函数，创建线程对象，但不启动
     * @param func 线程要执行的函数
     * @param name 线程名称，默认为空字符串
     * @details 只初始化对象状态，没有创建系统线程
     * 真正启动线程需要调用start()方法
     */
    explicit Thread(ThreadFunc func,const std::string& name=std::string());

    /**
     * @brief 析构函数
     * @details 如果线程已启动但未被join，则detach线程，避免线程对象销毁时资源泄漏
     * @note RAII原则，即使没有调用join()，线程也能安全结束
     */
    ~Thread();

    /**
     * @brief 启动函数
     * @details 流程：
     * 1.设置启动标志
     * 2.初始化信号量
     * 3.创建std::thread并执行lambda
     *      获取并缓存当前线程id
     *      发送信号量通知主线程
     *      执行函数func_
     * 4.等待信号量
     * @note sem的作用是同步主线程与新线程，等待新线程中将线程id设置完成后再继续执行
     * 避免外部调用tid()得到为初始化的值
     */
    void start();

    /**
     * @brief 等待线程结束
     * @details 阻塞当前线程，直到被调用线程执行完毕
     * 调用后joined_设置为true，避免析构时detach
     */
    void join();

    /**
     * @brief 判断线程是否已启动
     * @return true表示已启动，false表示为启动
     */
    bool started() const {return started_;}

    /**
     * @brief 获取系统级线程id
     * @return 内核线程id
     * @note start()中子线程创建后保证可以返回有效值
     */
    pid_t tid() const {return tid_;}

    /**
     * @brief 获取线程名称
     * @return 线程名称的常量引用
     */
    const std::string& name() const {return name_;}

    /**
     * @brief 使用静态成员变量计数，获取已创建的线程总数
     * @return 已创建的Thread对象数量
     */
    static int numCreated(){return numCreated_;}

private:
    /**
     * @brief 设置默认线程名称
     * @details 如果用户没有指定名称，生成格式为"ThreadX"的名称
     */
    void setDefaultName();

    /// 线程是否已经启动
    bool started_;

    /// 线程是否已被join
    bool joined_;

    /// 底层std::thread指针
    std::unique_ptr<std::thread> thread_;

    /// 系统线程id
    pid_t tid_;

    /// 线程要执行的函数
    ThreadFunc func_;

    /// 线程名称
    std::string name_;

    /// 已创建的Thread对象数量，原子变量
    static std::atomic_int numCreated_;
};

#endif //MUDUO_THREAD_H
