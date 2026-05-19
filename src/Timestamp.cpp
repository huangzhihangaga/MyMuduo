/**
 * @file Timestamp.cpp
 * @brief Timestamp类的实现
 */
#include "Timestamp.h"

#include <sys/time.h>
#include <time.h>

/// 每秒对应的微秒数
static const int kMicroSecondsPerSecond=1000*1000;

Timestamp::Timestamp():microSecondsSinceEpoch_(0) {}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch) :microSecondsSinceEpoch_(microSecondsSinceEpoch) {}

Timestamp Timestamp::now() {
    struct timeval tv;
    gettimeofday(&tv,nullptr);
    return Timestamp(tv.tv_sec*kMicroSecondsPerSecond + tv.tv_usec);
}

std::string Timestamp::toString() const {
    char buf[64];
    time_t seconds=static_cast<time_t>(microSecondsSinceEpoch_/kMicroSecondsPerSecond);
    struct tm tm_time;
    localtime_r(&seconds,&tm_time);
    snprintf(buf,sizeof(buf),"%4d/%02d/%02d %02d:%02d:%02d",
                                    tm_time.tm_year+1900,
                                    tm_time.tm_mon+1,
                                    tm_time.tm_mday,
                                    tm_time.tm_hour,
                                    tm_time.tm_min,
                                    tm_time.tm_sec);
    return buf;
}
