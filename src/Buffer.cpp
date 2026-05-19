/**
 * @file Buffer.cpp
 * @brief Buffer类的实现
 */

#include "Buffer.h"

#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>

ssize_t Buffer::readFd(int fd, int *saveErrno) {
    char extrabuf[65536]={0};
    struct iovec vec[2];
    const size_t writable=writableBytes();

    vec[0].iov_base=begin()+writerIndex_;
    vec[0].iov_len=writable;

    vec[1].iov_base=extrabuf;
    vec[1].iov_len=sizeof(extrabuf);

    // 如果buffer可写缓冲区足够大(>=64K)，则只用一个缓冲区，否则用两个
    const int iovcnt=(writable < sizeof(extrabuf)) ? 2:1;
    const ssize_t n=readv(fd,vec,iovcnt);

    if (n<0) {
        // 读取失败，保存错误码
        *saveErrno=errno;
    }else if (n<=writable) {
        // 数据全部读入buffer的可写区域，直接移动写指针
        writerIndex_+=n;
    }else {
        // 数据超过buffer的可写区域
        // 将buffer的可写区域填满
        writerIndex_=buffer_.size();

        // 把临时缓冲区中的数据追加到buffer末尾
        append(extrabuf,n-writable);
    }
    return n;
}

ssize_t Buffer::writeFd(int fd,int* saveErrno) {
    ssize_t n=write(fd,peek(),readableBytes());
    if (n<0) {
        *saveErrno=errno;
    }
    return n;
}
