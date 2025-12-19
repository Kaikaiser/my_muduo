#pragma once
#include "noncopyable.h"
#include <functional>
#include <memory>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <string>


class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>; 
    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();
    void start();
    void join();
    bool started() const {return started_;}
    bool joined() const {return joined_;}
    pid_t tid() const {return tid_;}
    const std::string& name() const {return name_;}
    static int numCreated() {return numCreated_;}

private:
    void setDefalutName();

    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;
    static std::atomic_int32_t numCreated_;


};