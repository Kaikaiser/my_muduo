#include "Socket.h"
#include "Logger.h"
#include "InetAddress.h"
#include <unistd.h>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
Socket::~Socket()
{
    close(sockfd_);
}


void Socket::bindAddress(const InetAddress &localaddr)
{
    if(0 != ::bind(sockfd_, (sockaddr*)localaddr.getSockAddr(), sizeof(sockaddr_in)))
    {
        LOG_FATAL("bind sockfd: %d failed \n", sockfd_);
    }
}

void Socket::listen()
{
    if(0 != ::listen(sockfd_, 1024))
    {
        LOG_FATAL("listen sockfd: %d failed \n", sockfd_);
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    /**
     * 问题1：acceptor参数不合法
     * 问题2：对返回的connfd没有设置非阻塞处理
     * 
    */
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    memset(&addr, 0, sizeof(addr));
    int connfd = ::accept4(sockfd_, (sockaddr*)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if(connfd >= 0)
    {
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    if(::shutdown(sockfd_, SHUT_WR))
    {
        LOG_ERROR("shutdownWrite error!");
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    // IPPROTO_TCP为协议级别
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)); 
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    // SQL_SOCKET为socket级别
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)); 
}

void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)); 
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)); 
}