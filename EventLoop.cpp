#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory>

// 防止一个线程创建多个EventLoop对象 __thraed => 线程局部变量(thread_local)
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用的超时时间
const int kPollTimes = 10000;

// 创建wakeupfd，用来notify唤醒subReactor处理新来的channel
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}


EventLoop::EventLoop() 
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd()) // main => sub
    , wakeupChannel_(new Channel(this, wakeupFd_))
    , currentActiveChannel_(nullptr)
{
    LOG_DEBUG("EventLoop create %p in thread %d \n", this, threadId_);
    if(t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件类型以及发生事件的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个eventloop都将监听wakeupChannel的EPOLLIN读事件了
    wakeupChannel_->enableReading();
}
EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}


// 开启事件循环
void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);
    while(!quit_)
    {
        activeChannels_.clear();
        // 主要用于监听两类的fd  一类是 client 的fd  一类是 wakeupfd
        pollReturnTime_ = poller_->poll(kPollTimes, &activeChannels_);

        for(Channel *channel : activeChannels_)
        {
            // Poller显示监听了哪些channel发生事件了，然后上报给EventLoop，通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        //执行当前EventLoop事件循环所进行的回调操作
        /*
        Io线程 mainloop 执行的是accept fd《=channel subloop 
        mainloop 提前注册一个回调cb（需要subloop来执行） wakeup subloop之后，要执行下面的方法，执行之前mainloop注册的回调cb
        */
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping \n", this);
    looping_ = false;

}

// 退出事件循环  
/**
 *                  mainloop
 * 
 *         muduo中使用的是轮询wakeup进行唤醒                   ==========可以使用消费者-生产者模式来保障线程安全==========
 * 
 *subloop1          subloop2         subloop3    .....  
*/

void EventLoop::quit()
{
    //1.loop在自己的线程中调用quit 
    quit_ = true;
    
    //2.在非loop自己的线程中，调用loop的quit  ==》one loop for one thread
    // 如果在其他线程中调用的是quit，  在一个subloop（工作线程）调用的是mainloop（IO线程）的quit
    if(!isInLoopThread())
    {
        wakeup();
    }

}


void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof(one));
    if(n != sizeof(one))
    {
        LOG_ERROR("EventLoop::handleRead() reads %d bytes insteadof 8", n);
    }
}

// 在当前loop中执行回调操作
void EventLoop::runInLoop(Functor cb)
{
    if(isInLoopThread()) // 是否在当前loop线程中，执行cb
    {
        cb();
    }
    else
    {
        queueInLoop(cb); // 在非当前loop线程中执行cb，需要先唤醒要执行的loop线程，再执行cb
    }
    
}

// 将cb放入queue中，唤醒loop所在的线程并执行cb操作
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    // 唤醒需要执行cb操作的loop线程
    // || callingPendingFunctors_-：为了防止前一个回调执行完后 loop写入新的回调 导致poll阻塞 所以写入数据wakeup当前loop线程
    if(!isInLoopThread() || callingPendingFunctors_) 
    {
        wakeup(); // 唤醒当前线程
    }
}

 
// 唤醒loop所在线程 向wakeupFd_写入一个数据 wakeupChannel发生读事件， 当前loop线程被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof(one));
    if(n != sizeof(one))
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8", n);
    }
}

// EventLoop中的方法 => Poller中的方法  
// 因为channel和poller是独立的 无法相互沟通 所以 channel只能通过Eventloop父组件来传递到poller进行沟通
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}


void EventLoop::doPendingFunctors() // 执行回调
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    for(const Functor &functor : functors)
    {
        functor(); // 执行当前loop所需的回调操作
    }
    callingPendingFunctors_ = false;
}