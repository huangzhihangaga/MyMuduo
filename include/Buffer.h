/**
 * @file Buffer.h
 * @brief 网络库的缓冲区类
 */

#ifndef MUDUO_BUFFER_H
#define MUDUO_BUFFER_H

#include <vector>
#include <string>
#include <algorithm>

/**
 * @brief 缓冲区类
 * @details 底层采用vector存储
 * 头部预留8bytes，便于添加协议头
 * 支持readv系统调用，使用栈上临时缓冲区减少数据拷贝
 * 内存布局：
 * | prependable bytes | readable bytes | writable bytes |
 *  0               readerIndex_     writerIndex_
 *  @note 非线程安全，需要外部加锁使用，但通常由EventLoop保证在同一线程使用
 */
class Buffer {
public:
    /// 前置空间大小 8bytes
    static const size_t kCheapPrepend=8;

    /// 初始化缓冲区大小 1024bytes
    static const size_t kInitialSize=1024;

    /**
     * @brief 构造函数
     * @param initialSize 初始缓冲区大小(不含前置空间)，默认1024bytes
     * @details 实际分配的内存大小为kCheapPrepend + initialSize
     * 读写指针初始都指向kCheapPrepend
     *
     */
    explicit Buffer(size_t initialSize=kInitialSize)
        :buffer_(kCheapPrepend+initialSize)
        ,readerIndex_(kCheapPrepend)
        ,writerIndex_(kCheapPrepend){}

    /**
     * @brief 获取可读字节数
     * @return 待读取的数据长度
     */
    size_t readableBytes() const {
        return writerIndex_-readerIndex_;
    }

    /**
     * @brief 获取可写字节数
     * @return 剩余可写入空间大小
     */
    size_t writableBytes() const {
        return buffer_.size()-writerIndex_;
    }

    /**
     * @brief 获取前置空间大小
     * @return readIndex_前由多少字节(可用于写入协议头)
     */
    size_t prependableBytes() const {
        return readerIndex_;
    }

    /**
     * @brief 获取读指针位置
     * @return 指向可读数据起始位置的指针
     */
    const char* peek() const {
        return begin()+readerIndex_;
    }

    /**
     * @brief 从缓冲区中取出指定长度的数据
     * @param len 要取出的数据长度
     * @details 只移动读指针，不删除数据
     * 如果len>=可读字节数，则清空整个缓冲区
     */
    void retrieve(size_t len) {
        if (len<readableBytes()) {
            readerIndex_+=len;
        }else {
            retrieveAll();
        }
    }

    /**
     * @brief 清空缓冲区
     * @details 将读写指针都重置到kCheapPrepend位置
     */
    void retrieveAll() {
        readerIndex_=writerIndex_=kCheapPrepend;
    }

    /**
     * @brief 取出所有可读数据并转换为字符串
     * @return 包含所有可读数据的字符串
     */
    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }

    /**
     * @brief 取出指定长度的数据并转为字符串
     * @param len 要取出数据的长度
     * @return 包含指定长度数据的字符串
     */
    std::string retrieveAsString(size_t len) {
        std::string result(peek(),len);
        retrieve(len);
        return result;
    }

    /**
     * @brief 去报缓冲区至少由len字节的可写空间
     * @param len 需要保证的可写空间大小
     * @details 如果空间不足，调用makeSpace()扩容
     */
    void ensureWriteableBytes(size_t len) {
        if (writableBytes()<len) {
            makeSpace(len);
        }
    }

    /**
     * @brief 向缓冲区追加数据
     * @param data 要追加数据的指针
     * @param len 数据长度
     */
    void append(const char* data,size_t len) {
        ensureWriteableBytes(len);
        std::copy(data,data+len,beginWrite());
        writerIndex_+=len;
    }

    /**
     * @brief 获取可写起始地址
     * @return 指向可写地址起始的指针
     */
    char* beginWrite() {
        return begin()+writerIndex_;
    }

    /**
     * @brief 获取可写起始地址，常量
     * @return 指向可写地址起始的常量指针
     */
    const char* beginWrite() const {
        return begin()+writerIndex_;
    }

    /**
     * @brief 从文件描述符读取数据到缓冲区
     * @param fd 文件描述符
     * @param saveErrno 保持errno的输出参数
     * @return 读取的字节数，失败返回-1
     * @details 使用readv，优先读入缓冲区
     * 如果缓冲区空间不足。则使用栈上的extrabuf作为临时缓冲区，减少系统调用次数
     */
    ssize_t readFd(int fd,int* saveErrno);

    /**
     * @brief 将缓冲区数据写入文件描述符
     * @param fd 文件描述符
     * @param saveErrno 保持errno的输出参数
     * @return 写入的字节数，失败返回-1
     */
    ssize_t writeFd(int fd,int* saveErrno);

private:
    /**
     * @brief 获取底层数据的起始位置
     * @return 指向vector第一个元素的指针
     */
    char* begin() {
        return &*buffer_.begin();
    }

    /**
     * @brief 获取底层数据的起始位置，常量版本
     * @return 指向vector第一个元素的常量指针
     */
    const char* begin() const {
        return &*buffer_.begin();
    }

    /**
     * @brief 扩展缓冲区容量，确保至少由len字节的可写空间
     * @param len 需要保证可写空间的长度
     * @details 扩展策略：
     *          如果：
     *              可写空间 + 前置空间 < len + kCheapPrepend
     *              即 可写空间 + 已被读出数据的空间 < len，直接扩容vector
     *          否则：
     *              将可读数据移动到预留头部之后，合并 已读出数据的空间和可写空间
     */
    void makeSpace(size_t len) {
        if (writableBytes()+prependableBytes() < len+kCheapPrepend) {
            buffer_.resize(writerIndex_+len);
        }else {
            size_t readable=readableBytes();
            std::copy(begin()+readerIndex_,begin()+writerIndex_,begin()+kCheapPrepend);
            readerIndex_=kCheapPrepend;
            writerIndex_=readerIndex_+readable;
        }
    }

    /// 底层存储，用vector可自动管理内存
    std::vector<char> buffer_;

    /// 读指针位置，指向待读取数据的起始位置
    size_t readerIndex_;

    /// 学指针位置，指向待写入数据的起始位置
    size_t writerIndex_;
};


#endif //MUDUO_BUFFER_H
