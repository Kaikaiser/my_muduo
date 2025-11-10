#pragma once 

#include "noncopyable.h"
#include "TimeStamp.h"
#include "CurrentThread.h"

#include <functional>
#include <string>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

class Channel;
class Poller;

// 时间循环类 主要包括 两个大模块  Channel和Poller Poller可以看成epoll的抽象
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开始事件循环
    void loop();

    // 退出事件循环
    void quit();

    TimeStamp pollReturnTime() const {return pollReturnTime_;}

    // 在当前loop中执行
    void runInLoop(Functor cb);

    // 把cb放入队列中，唤醒loop所在的线程，执行cb
    void queueInLoop(Functor cb);

    // 唤醒loop所在线程
    void wakeup();

    // EventLoop中的方法 => Poller中的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);
    
    // 判断EventLoop的对象是否在当前线程里面
    bool isInLoopThread() const {return threadId_ == CurrentThraedId::tid();}

private:
    void handleRead(); // wake up
    void doPendingFunctors(); // 执行回调
    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_; // 原子操作，通过CAS操作实现
    std::atomic_bool quit_; // 标识退出loop循环
   
    const pid_t threadId_; // 记录当前EventLoop所在线程的id

    TimeStamp pollReturnTime_; // 记录poller返回发生事件的channels的时间点
    std::unique_ptr<Poller> poller_;

    // muduo库使用的eventfd来进行mainloop和subloop之间的通信
    int wakeupFd_; //主要作用：当mainloop获取一个新用户的channel，通过轮询算法选择一个subloop，通过该成员唤醒subloop处理channel
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;
    Channel *currentActiveChannel_;

    std::atomic_bool callingPendingFunctors_; // 标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_; // 存储loop需要执行的所有回调操作
    std::mutex mutex_; // 互斥锁，用来保护上面vector容器的线程安全操作
};  