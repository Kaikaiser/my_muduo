#include "Acceptor.h"
#include "Logger.h"
#include "EventLoop.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <cerror>

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if(sockfd < 0)
    {
        LOG_FATAL("%s: %s: %d: listen socket create error:%d\n", __FILE__, __func__, __LINE__, errno);
    }
}


Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop)
    , acceptSocket_(createNonblocking())
    , acceptChannel_(loop, acceptSocket_.fd())
    , listening_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
}

Acceptor::~Acceptor()
{

}


void Acceptor::listen()
{

}

void Acceptor::handleRead()
{

}