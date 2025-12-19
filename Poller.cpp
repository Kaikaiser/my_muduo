#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop *loop) : ownerLoop_(loop)
{
}


// 判断参数channel是否在当前poller中
bool Poller::hasChannel(Channel *channel) const
{
    // find迭代器找不到返回end 找到返回对应的迭代器
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}