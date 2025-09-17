#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop *loop) : ownerLoop_(loop)
{
}
virtual ~Poller();

// 给所有的io复用保留统一的接口
virtual TimeStamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
virtual void updateChannel(Channel *channel) = 0;
virtual void removeChannel(Channel *channel) = 0;
// 判断参数channel是否在当前poller中
bool Poller::hasChannel(Channel *channel) const
{
    // find迭代器找不到返回end 找到返回对应的迭代器
    auto it = channel_.find(channel->fd());
    return it != channel_.end() && it->second == channel;
}