#include"Timestamp.h"
#include<time.h>

TimeStamp::TimeStamp()
{

}
TimeStamp::TimeStamp(int64_t microSecondsSinceEpoch)
    : microSecondsSinceEpoch_(microSecondsSinceEpoch){}

TimeStamp TimeStamp::now()
{
    time_t t1 = time(NULL)
    return TimeStamp(t1);
}   
// 只读方法 不允许修改值 将now获取的时间转化为字符串
std::string toString() const
{
    char buf[128] = {0};
    tm *tm_time = localtime(&microSecondsSinceEpoch_);
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d", 
            tm_time->tm_year + 1900,
            tm_time->tm_mom + 1,
            tm_time->tm_mday,
            tm_time->tm_hour,
            tm_time->tm_min,
            tm_time->tm_sec);
    return buf;
}


// 测试
// #include<iostream>
// int mian()
// {
//     std::cout<<TimeStamp::now().toString()<<std::endl;

//     return 0;
// }