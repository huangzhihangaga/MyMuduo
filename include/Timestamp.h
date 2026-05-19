/**
 * @file Timestamp.h
 * @brief 时间戳类，封装Unix时间戳，微秒精度
 */

#ifndef MUDUO_TIMESTAMP_H
#define MUDUO_TIMESTAMP_H
#include <cstdint>
#include <iostream>
#include <string>

/**
 * @brief 时间戳类，表示从 1970-01-01 00:00:00 UTC 开始的微妙数
 */
class Timestamp {
public:
    /**
     * @brief 默认构造函数
     * @details 默认微秒数为0
     */
    Timestamp();

    /**
     * @brief 从微秒数构造时间戳
     * @param microSecondsSinceEpoch 自Unix纪元以来的微秒数
     * @note explicit 关键字防止隐式类型转换
     */
    explicit Timestamp(int64_t microSecondsSinceEpoch);

    /**
     * @brief 获取当前时间的时间戳
     * @return 当前时刻的Timestamp对象
     * @note 使用gettimeofday()实现，精度为秒级
     */
    static Timestamp now();

    /**
     * @brief 将时间戳转换为可读的字符串格式
     * @return 格式化后的时间字符串，例如"YYYY/MM/DD HH:MM:SS"
     * @note 只显示到秒数，微妙部分被舍弃
     */
    std::string toString() const;

private:
    /// 从 1970-01-01 00:00:00 UTC 开始的微秒数
    int64_t microSecondsSinceEpoch_;
};

#endif //MUDUO_TIMESTAMP_H
