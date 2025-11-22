#pragma once 
/**
 *  用户使用muduo编写服务器程序
*/
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include <functional>
#include <string>
#include <memory>

// 对外的服务器编程使用的类

class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
private:
    EventLoop *loop_; //baseLoop 用户定义的loop
    const std::string inPort_;
    const std::string name _;
    std::unique_ptr<Acceptor> acceptor_;
    std::shared_ptr<EventLoopThreadPool> threadPool_;  // one loop per thread
    ConnectionCallback connectionCallback_;  // 有新连接时的回调
    MessageCallback messageCallback_; // 有读写消息时的回调
}; 