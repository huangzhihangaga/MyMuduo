//
// Created by 小晓 on 2026/4/7.
//

#ifndef MUDUO_NONCOPYABLE_H
#define MUDUO_NONCOPYABLE_H

class noncopyable {
public:
    noncopyable(const noncopyable&)=delete;
    noncopyable& operator=(const noncopyable&)=delete;

protected:
    noncopyable()=default;
    ~noncopyable()=default;
};


#endif //MUDUO_NONCOPYABLE_H
