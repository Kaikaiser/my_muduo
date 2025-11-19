#include "Thread.h"
#include "CurrentThread.h"
#include <semaphore.h>

// 这里是使用atomic的拷贝构造 不能直接赋值
static std::atomic_int32_t Thread::numCreated_(0);
// 参数默认值只出现一个地方即可
Thread::Thread(ThreadFunc func , const std::string &name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name)
{
    setDefalutName();
}

Thread::~Thread()
{
    if(started_ && !joined_)
    {
        thread_->detach(); // Thread类设置了分离线程的方法
    }
}

void Thread::start()  // 一个thread对象就是记录的一个新线程的详细信息
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);

    // 开启线程
    thread_ = std::shared_ptr<std::thread>((new std::thread)[&](){
        // 获取线程id
        tid_ = CurrentThread::tid();
        sem_post(&sem);
        func_();
    });

    // 这里必须等待上面新创建的线程获取tid值
    sem_wait(&sem);

}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}


void Thread::setDefalutName()
{
    int num = ++numCreated_;
    if(name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "Thread%d", num);
        name_ = buf;
    }

}