//
// Created by 小晓 on 2026/4/7.
//

#ifndef MUDUO_TIMESTAMP_H
#define MUDUO_TIMESTAMP_H
#include <cstdint>
#include <iostream>
#include <string>

class Timestamp {
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    static Timestamp now();
    std::string toString() const;

private:
    int64_t microSecondsSinceEpoch_;
};

#endif //MUDUO_TIMESTAMP_H
