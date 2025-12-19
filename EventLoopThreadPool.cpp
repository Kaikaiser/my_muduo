#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include <memory>


EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg)
    : baseLoop_(baseLoop)
    , name_(nameArg)
    , started_(false)
    , numThreads_(0)
    , next_(0)
{
}
EventLoopThreadPool::~EventLoopThreadPool()
{
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb) 
{
    started_ = true;
    // 未设置numThread 
    for(int i=0; i<numThreads_; ++i)
    {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof(buf), "%s%d", name_.c_str(), i);
        EventLoopThread *t = new EventLoopThread(cb, buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->StartLoop()); // 底层创建线程，绑定一个新EventLoop, 返回该loop的地址
    }

    // 整个服务端只有一个线程，运行baseLoop
    if(numThreads_ == 0 && cb)
    {
        cb(baseLoop_);
    }
}

// 如果在工作在多线程中，baseloop_默认轮询的方式分配channe给subloop
// baseloop一般指io线程，处理新用户连接事件、读写事件等等  工作线程处理业务逻辑   但是如果没有新创建工作线程  baseloop就io线程和工作线程都用
EventLoop* EventLoopThreadPool::getNextLoop()
{
    EventLoop *loop = baseLoop_;

    if(!loops_.empty()) // 通过轮询来获取下一个处理事件loop
    {
        loop = loops_[next_];
        ++next_;
        if(next_ >= loops_.size())
        {
            next_ = 0;
        } 
    }
    return loop;

}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
    if(loops_.empty())
    {
        return std::vector<EventLoop*>(1, baseLoop_);
    }
    else{
        return loops_;
    }
}