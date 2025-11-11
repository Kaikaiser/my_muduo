#include "EventLoopThread.h"
#include "EventLoop.h"
#include <memory>


EventLoopThread::EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(), 
                const std::string &name = std::string())
                : loop_(nullptr)
                , exiting_(false)
                , thread_(std::bind(&EventLoopThread::ThreadFunc, this), name)
                , mutex_()
                , cond_()
                , callback(cb)
{
}


EventLoopThread::~EventLoopThread();
{
    exiting_ = true;
    if(loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }

}

// 相当于主线程等待子线层初始化完成
EventLoop* EventLoopThread::StartLoop()
{
    thread_.start(); // 启动底层的新线程
    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr)
        {
            cond_.wait(lock); // 等待子线程通知  wait的作用是 自动解锁然后阻塞当前线程 直到被唤醒  唤醒后又加锁
        }
        loop = loop_;
    }
    return loop;

}

// 下面这个新方法 在单独的新线程里面运行
void EventLoopThread::ThreadFunc()
{
    EventLoop loop; // 创建一个单独的eventloop 和上述的线程是一一对应的 one loop per thread
    if(callback_)
    {
        callback_(&loop);
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    loop.loop(); // EventLoop loop =》 Poller.poll
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;

}