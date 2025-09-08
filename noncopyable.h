#pragma once 

/*
*noncopyable类被继承后，派生类对象可以正常的进行构造和析构，但是派生类对象无法进行
*拷贝构造和赋值操作，因为在基类就删除的拷贝构造和赋值操作（构造时 先构造基类 然后派生类 析构时相反）
*/
class noncopyable
{
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;

private:
    noncopyable() = default;
    ~noncopyable() = default;

};