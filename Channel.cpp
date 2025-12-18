#include "Channel.h"
#include <sys/epoll.h>
#include "EventLoop.h"
#include "Logger.h"
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

// channel的tie方法是什么时候调用？ 一个TcpConnection新连接创建的时候（底层绑定一个channel） TcpConnection -> channel
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

/*
* 当改变Channel 表示的fd 的感兴趣events事件后，update负责在Poller 里面
* 更改fd 相应事件epoll_ctl
* channel和poller是独立的 通过他们所属的EventLoop进行通信
*/ 
void Channel::update()
{
    // 通过channel所属EventLoop 调用poller相应的方法 注册fd的events事件
    loop_->updateChannel(this);
}

// 在Channel所属的EventLoop中，把当前Channel删除
void Channel::remove()
{
    loop_->removeChannel(this); this是传入自己channel
}
// fd得到poller通知后，处理相应事件的函数
void Channel::handleEvent(TimeStamp receiveTime)
{
    // 如果channel中的tie_有绑定对象
    if(tied_)
    {
        // 这里的lock用于提升为强智能指针 返回不为空即为提升成功 否则返回nullptr
        std::shared_ptr<void> guard = tie_.lock(); 
        if(guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

// 根据poller通知的channel的具体事件，channel调用具体事件的回调函数 log日志还可以加 _FUNCTION_  _LINE_
void Channel::handleEventWithGuard(TimeStamp receiveTime)
{
    LOG_INFO("channel handleEvent revents:%d\n", revents_);

    // 出问题 发生异常
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if(closeCallback_)
        {
            closeCallback_();
        }
    }

    if(revents_ & EPOLLERR)
    {
        if(errorCallback_)
        {
            errorCallback_();
        }
    }

    if(revents_ & (EPOLLIN | EPOLLPRI))
    {
        if(readCallback_)
        {
            readCallback_(receiveTime);
        }
    }

    if(revents_ & EPOLLOUT)
    {
        if(writeCallback_)
        {
            writeCallback_();
        }
    }

}