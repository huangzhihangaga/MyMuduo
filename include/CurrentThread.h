/**
 * @file CurrentThread.h
 * @brief 当前线程信息获取模块，提供线程id的缓存机制
 */

#ifndef MUDUO_CURRENTTHREAD_H
#define MUDUO_CURRENTTHREAD_H

#include <unistd.h>
#include <unistd.h>
#include <sys/syscall.h>

/**
 * @brief 当前线程相关的函数和变量
 * @details 提供获取当前线程id的功能
 * 使用thread_local线程局部变量存储线程id，避免频繁系统调用带来性能开销
 */
namespace CurrentThread {
    /**
     * @brief 缓存的当前的线程id，通过线程局部变量存储
     * @details 每个线程拥有独立的t_cachedTid副本，初始值为0表示未初始化
     * 首次调用tid()通过系统调用获取真实的线程id并缓存，后续调用直接返回缓存值
     */
    extern thread_local int t_cachedTid;

    /**
     * @brief 缓存当前线程id
     * @details 通过系统调用SYS_gettid获取Linux内核线程id并存储到线程局部变量t_cachedTid中
     * @note 该函数每个线程只会调用一次，在首次调用tid()时
     * 后续直接使用缓存值，避免重复的系统调用
     */
    void cacheTid();

    /**
     * @brief 获取当前线程id，如果缓存中有则直接从缓存中获取
     * @return 当前线程的linux内核线程id
     * @details 首次调用时t_cachedTid==0，调用cacheTid()获取真实id并缓存
     * 后续调用共直接返回缓存值，不需要再次进行系统调用
     * __builtin_expect分支预测优化：
     * __builtin_expect(exp,c) 表示exp==c的可能性极大
     * 帮助cpu预取指令，提高分支预测的命中率
     */
    inline int tid() {
        /// 表达式t_cachedTid==0的值很可能为0
        if (__builtin_expect(t_cachedTid==0,0)) {
            cacheTid();
        }
        return t_cachedTid;
    }

}

#endif //MUDUO_CURRENTTHREAD_H
