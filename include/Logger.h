/**
 * @file Logger.h
 * @brief 日志记录器类
 */

#ifndef MUDUO_LOGGER_H
#define MUDUO_LOGGER_H

#include <string>

#include "noncopyable.h"

/**
 * @def LOG_INFO
 * @brief 输出INFO级别的日志信息
 * @param logmsgFormat 格式化字符串
 * @param ... 可变参数，与格式化字符串匹配
 * @note 使用do while(0)包裹，确保宏可以安全展开
 */
#define LOG_INFO(logmsgFormat,...) \
    do \
    { \
        Logger& logger=Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024]={0}; \
        snprintf(buf,1024,logmsgFormat,##__VA_ARGS__); \
        logger.log(buf); \
    } while (0) \

/**
 * @def LOG_ERROR
 * @brief 输出ERROR级别的日志信息
 * @param logmsgFormat 格式化字符串
 * @param ... 可变参数，与格式化字符串匹配
 * @note 使用do while(0)包裹，确保宏可以安全展开，ERROR级别不会终止程序允许
 */
#define LOG_ERROR(logmsgFormat,...) \
    do \
    { \
        Logger& logger=Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024]={0}; \
        snprintf(buf,1024,logmsgFormat,##__VA_ARGS__); \
        logger.log(buf); \
    } while (0) \

/**
 * @def LOG_FATAL
 * @brief 输出FATAL级别的日志信息
 * @param logmsgFormat 格式化字符串
 * @param ... 可变参数，与格式化字符串匹配
 * @note 使用do while(0)包裹，确保宏可以安全展开, 调用后程序会退出，不会执行后续代码
 */
#define LOG_FATAL(logmsgFormat,...) \
    do \
    { \
        Logger& logger=Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024]={0}; \
        snprintf(buf,1024,logmsgFormat,##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    } while (0) \

/**
 * @def LOG_DEBUG
 * @brief 输出DEBUG级别的日志信息
 * @param logmsgFormat 格式化字符串
 * @param ... 可变参数，与格式化字符串匹配
 * @note 使用do while(0)包裹，确保宏可以安全展开，只有在定义了MUDEBUG宏是才输出日志，负责该宏展开为空
 */
#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat,...) \
    do \
        { \
        Logger& logger=Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024]={0}; \
        snprintf(buf,1024,logmsgFormat,##__VA_ARGS__); \
        logger.log(buf); \
    } while (0)
#else
    #define LOG_DEBUG(logmsgFormat,...)
#endif

/**
 * @enum LogLevel
 * @brief 日志级别枚举
 * @details 定义了四个日志级别
 * DEBUG 调试信息
 * INFO 调试信息
 * ERROR 错误信息
 * FATAL 致命错误，会导致程序退出
 */
enum LogLevel {
    INFO,
    ERROR,
    FATAL,
    DEBUG
};

/**
 * @brief 日志记录其类，采用单例模式
 */
class Logger : noncopyable {
public:
    /**
     * @brief 静态成员函数，获取Logger单例
     * @return Logger对象的引用
     */
    static Logger& instance();

    /**
     * @brief 设置当前日志级别
     * @param level 对应LogLevel的日志级别
     */
    void setLogLevel(int level);

    /**
     * @brief 输出日志信息
     * @param msg 要输出的日志内容
     */
    void log(std::string msg);
private:
    /// 当前日志级别
    int logLevel_;

    /**
     * @brief 私有构造函数，用于实现单例模式
     * @details 构造函数被设置为私有，防止外部直接创建实例，保证只能通过instance()方法获取实例
     * 设置默认日志级别为INFO
     */
    Logger():logLevel_(INFO){}
};

#endif //MUDUO_LOGGER_H
