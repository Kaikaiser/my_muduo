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

    // 给所有的io复用保留统一的接口
    virtual TimeStamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;
    // 判断参数channel是否在当前poller中
    bool hasChannel(Channel *channel) const;

    // EventLoop 可以通过该接口获取默认的io复用的具体实现
    static Poller* newDefaultPoller(EventLoop *loop);
protected:
    // 这里面的key -> sockfd   value -> sockfd对应的channel类型 
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;
private:
    EventLoop *ownerLoop_; // 定义Poller所属的事件循环EventLoop

};