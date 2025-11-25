#include "TcpServer.h"
#include "Logger.h"
#include <string>
#include <functional>

EventLoop* CheckLoopNotNull(EventLoop *loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s: %s: %d mainloop does not exist! \n", __FILE__, __func__, __LINE__);
    }
    return loop;
}


TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &nameArg, Option option = kNOReusePort)
        : loop_(CheckLoopNotNull(loop))
        , inPort_(listenAddr.toIpPort())
        , name_(nameArg)
        , acceptor_(new Acceptor(loop, listenAddr, option == kReusePort))
        , thread_(new EventLoopThreadPool(loop, name_))
        , connectionCallback_()
        , messageCallback_()
        , nextConnId_(1)
{
    // 当有新用户连接时， 会执行TcpServer::newConnection回调操作
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
    
}

TcpServer::~TcpServer()
{
    
}

// 设置底层subloop的个数
void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

// 开启服务器监听 -> acceptor -> listen 
void TcpServer::start()
{
    if(started_++ == 0)  // 防止一个TcpServer对象被start多次
    {
        threadPool_->start(threadInitCallback_); // 启动底层的loop线程池
        // acceptor_是智能指针  使用智能指针本身函数用. ->因为本身他是一个对象  如果要使用智能指针内部指针的函数 即为-> 
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get())); 
    }
}

void newConnection(int sockfd, const InetAddress &peerAddr);