#include "Channel.h"
#include<sys/epoll.h>
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

// 一个EventLoop：很多个Channel ->ChannelList + 一个 Poller
Channel::Channel(EventLoop *loop, int fd) 
    : loop_(loop)
    , fd_(fd)
    , events_(0) // fd 感兴趣的事件 注册到Poller里的事件
    , revents_(0) // Poller返回的事件
    , index_(-1)
    , tied_(false)
{   
}

Channel::~Channel()
{
}

void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

/*
* 当改变Channel 表示的fd 的感兴趣events事件后，update负责在Poller 里面
* 更改fd 相应事件epoll_ctl
*/ 
void Channel::update()
{

}
void handleEvent(TimeStamp receiveTime);