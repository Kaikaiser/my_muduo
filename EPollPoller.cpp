#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include<errno.h>
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

TimeStamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{

}


// channel update remove => EventLoop updateChannel removeChannel => poller updateChannel removeChannel
/*
*                EventLoop
*   ChannelList           Poller
*                  ChannnelMap <fd, channel*>   epollfd
*/
void EPollPoller::updateChannel(Channel *channel)  // -->eopll_ctl
{
    const int index = channel->index();
    LOG_INFO("fd=%d events=%d index=%d \n", channel->fd(), channel_>events(), channel->index());
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
    // 从epoll中删除
    int index = channel->index();
    if(index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);

    }
    channel->set_index(kNew);
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
// 更新channel通道 epoll_ctl_(add/del/mod)
void EPollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));

    int fd = channel->fd();
    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;
    int fd = channel->fd();
    if(::epoll_ctl(epollfd_, operation, fd, &event)<0)
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


EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}