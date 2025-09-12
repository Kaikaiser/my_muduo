#pragma once
#include<vector>
#include<sys/epoll.h>
#include "Poller.h"
#include "TimeStamp.h"
/*
* epoll的使用
* epoll_create 创建epollfd(句柄)
* epoll_ctl 添加epoll想要监听的fd, 以及针对这个fd所感兴趣的事件  add/mod/del
* epoll_wait 
*/


class Channel;
class EPollPoller : public Poller 
{
public:
    EPollPoller(EventLoop *loop); //-->epoll_create
    ~EPollPoller() override;

    // 重写基类poller的抽象方法
    TimeStamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;  // -->eopll_ctl
    void removeChannel(Channel *channel) override;  // -->eopll_ctl

private:
    // const(constexpr)可以用于类内定义static 但是要是int类型或者其他可迭代类型 string不行
    static const int kInitEventListSize = 16;

    // 填写活跃的channel
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    // 更新channel通道
    void update(int operation, Channel *channel);
    using EventList = std::vector<epoll_event>;

    int epollfd_;
    EventList events_;
};