#pragma once 
#include "noncopyable.h"
#include "TimeStamp.h"
#include<functional>
#include<memory>
/*
* 理清楚 EventLoop、Channel、Poller之间的关系  在Reactor模型图上对应 Demultiplex
* Channel 理解为通道 封装了sockfd 和其感兴趣的事件event，如EPOLLIN 和EPOLLOUT事件
* 还绑定了poller返回的具体事件
*/

class EventLoop;

class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(TimeStamp)>;


    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到poller通知以后处理事件，调用相应的方法
    void handleEvent(TimeStamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb){ readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb){ writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb){ closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb){ errorCallback_ = std::move(cb); }

    // 防止当channel被手动remove掉之后，channel还在进行回调操作
    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_;}
    int events() const { return events_;}
    void set_revents(int revt) { revents_ = revt;}
    bool isNoneEvent() const { return events_ == kNoneEvent;}

    // 设置fd相应的事件状态  |表示加入时间  &~表示移除事件
    void enableReading() { events_ |= kReadEvent; update(); }   // 加入读事件
    void disableReading() { events_ &= ~kReadEvent; update(); } // 移除读事件
    void enableWriting() { events_ |= kWriteEvent; update(); }  // 加入写事件
    void disableWriting() { events_ &= ~kWriteEvent; update(); } // 移除写事件
    void disableAll() { events_ = kNoneEvent; update(); }       // 移除所有

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; } // 判断无事件
    bool isWriting() const { return events_ & kWriteEvent; }   // &用于判断是否有可写事件
    bool isReading() const { return events_ & kReadEvent; }    // &用于判断是否有可读事件

    int index() const { return index_;}
    void set_index(int idx) { return index_ = idx;} 

    // one loop per thread
    EventLoop* ownerLoop(){ return loop_; }
    void remove(); // 删除channel

private:
    void update();
    void handleEventWithGuard(TimeStamp receiveTime);


    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; // 事件循环
    const int fd_;    // fd, poller监听的对象
    int events_;      // 注册fd的感兴趣的事件
    int revents_;     // poller返回的具体事件
    int index_;       //

    // 使用weak_ptr避免循环引用 只是用做观察（观察对象是否存活） 后面使用shared_ptr来进行对对象的使用
    // weak_ptr不能* 或 -> 访问对象 先通过.lock()升级为shared_ptr进行使用
    std::weak_ptr<void> tie_; 
    bool tied_;

    // 因为channel通道中可以获得fd最终发生的事件revents，所以他负责调用相应的回调函数操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;

};