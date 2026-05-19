/**
 * @file noncopyable.h
 * @brief 禁止拷贝的基类，用于派生类继承以禁用拷贝构造和拷贝赋值
 */

#ifndef MUDUO_NONCOPYABLE_H
#define MUDUO_NONCOPYABLE_H

/**
 * @brief 禁止拷贝的基类
 * @details 继承noncopyable的类将自动禁用拷贝构造和拷贝赋值运算符
 */
class noncopyable {
public:
    /**
     * @brief 删除拷贝构造函数
     */
    noncopyable(const noncopyable&)=delete;

    /**
     * @brief 删除拷贝赋值运算符
     */
    noncopyable& operator=(const noncopyable&)=delete;

protected:
    /**
     * @brief 默认构造函数 protected 仅允许派生类构造
     */
    noncopyable()=default;

    /**
     * @brief 默认析构函数 protected 多态中可以正确释放资源
     * @details noncopyable虽然作为基类，但是不会通过基类指针指向派生类对象
     * 不需要加virtual，可以避免虚函数带来的消耗
     * 同时设置为protected，避免外部错误使用基类指针删除派生类对象
     */
    ~noncopyable()=default;
};


#endif //MUDUO_NONCOPYABLE_H
