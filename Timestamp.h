#pragma once 
#include<iostream>
#include<string>
class TimeStamp
{
public:
    TimeStamp();
    explicit TimeStamp(int64_t microSecondsSinceEpoch);
    static TimeStamp now();
    // 只读方法 不允许修改值 将now获取的时间转化为字符串
    std::string toString() const;
private:
    int64_t microSecondsSinceEpoch_;
};
