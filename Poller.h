#pragma once 

#include "noncopyable.h"
#include "TimeStamp.h"
#include<vector>
#include<unordered_map>

//只是用于声明而不创建对象实例的话 不用引入Channel.h头文件 防止暴露更多的信息
class Channel;
class EventLoop;
// muduo库中多路时间分发器的核心  io复用模块 负责事件监听（epoll_wait）
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;
    Poller(EventLoop *loop);
    virtual ~Poller();

    virtual TimeStamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
protected:
    using ChannelMap = std::unordered_map<int, Channel*>;
private:
    EventLoop *ownerLoop_; // 定义Poller所属的事件循环EventLoop

};