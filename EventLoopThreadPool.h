#pragma once 
#include "noncopyable.h"
#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoopThread;
class EventLoop;

class EventLoopThreadPool : noncopyable
{
public:
    using ThreadInitFunc = std::function<void(EventLoop*)>;

private:
    EventLoop *baseLoop_;
    std::string name_;  // EventLoop loop;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread*>> threads_;
    std::vector<EventLoop*> loops_;

}