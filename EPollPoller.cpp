#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
// channel未添加到poller中 
const int kNew = -1;  // channel的初始化成员index_ 也为-1
// channel已添加到poller中
const int kAdded = 1;
// channel从poller中删除
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop *loop) 
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize) // vector<epoll_event>
{
    if(epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}


EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

TimeStamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    // DEBUG输出更合理  INFO会降低效率
    LOG_INFO("func = %s => fd total count: %lu \n", __FUNCTION__, channels_.size());
    //LOG_DEBUG("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel_>events(), channel->index());
    
    // vector的begin和end返回的是迭代器iterator 可以用*解引用后再取地址&拿到指针地址
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    TimeStamp now(TimeStamp::now());
    if(numEvents > 0)
    {
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if(numEvents == events_.size())
        {
            events_.resize(events_.size()*2);
        }
    }
    else if(numEvents == 0)
    {
        LOG_DEBUG("%s timeout \n", __FUNCTION__);
    }
    else
    {
        if(saveErrno != EINTR)
        {
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err! \n");
        }
    }
    return now;
}


// channel update remove => EventLoop updateChannel removeChannel => poller updateChannel removeChannel
/*
*                EventLoop  =>  poller.poll
*   ChannelList           Poller => 抽象基类 可以实现指向不同的poller派生类（不同io复用对象 => epoll poll select）
*                  ChannnelMap <fd, channel*>   epollfd
*/
void EPollPoller::updateChannel(Channel *channel)  // -->eopll_ctl
{
    const int index = channel->index();
    LOG_INFO("func = %s => fd = %d events = %d index = %d \n", __FUNCTION__, channel->fd(), channel->events(), channel->index());
    if(index ==kNew || index == kDeleted)
    {
        if(index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else  //表示channel已经在poller上注册过了
    {
        int fd = channel->fd();
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 从poller中删除channel 1.从poller的channelMap中删除 2.从epoll中删除
void EPollPoller::removeChannel(Channel *channel)  // -->eopll_ctl
{
    // 从poller的channelMap中删除
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("func = %s => fd = %d  \n", __FUNCTION__, fd);
    // 从epoll中删除
    int index = channel->index();
    if(index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);

    }
    channel->set_index(kNew);
}

// 填写活跃的channel的连接
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for(int i=0; i<numEvents; ++i)
    {
        // epoll_event的data.ptr为void*类型 要强制转换为Chanel*
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel); // EventLoop拿到了它的poller给它返回的所有的发生事件的channel列表 => activeChannels
    }
}

// 更新channel通道 epoll_ctl_(add/del/mod)
void EPollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));

    int fd = channel->fd();
    // 这里的event的fd和ptr是发生事件的fd和对应的channel指针但是这里的ptr是void*类型
    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;

    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if(operation == EPOLL_CTL_DEL)
        {
            if(operation == EPOLL_CTL_DEL)
            {
                LOG_ERROR("epoll_ctl_del error:%d\n", errno);
            }
            else
            {
                LOG_FATAL("epoll_ctl_add/mod error:%d\n", errno);
            }
        }
    }

}

