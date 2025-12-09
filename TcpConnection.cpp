#include "TcpConnection.h"
#include "Logger.h"
#include "Channel.h"
#include "EventLoop.h"

#include <errno.h>
#include <memory>



static EventLoop* CheckLoopNotNull(EventLoop *loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s: %s: %d mainloop does not exist! \n", __FILE__, __func__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                const std::string &nameAgr,
                int sockfd,
                const InetAddress &localAddr,
                const InetAddress &peerAddr)
    : loop_(CheckLoopNotNull(loop))
    , name_(nameAgr)
    , state_(kConnecting)
    , reading_(true)
    , socket_(new Socket(sockfd))
    , channel_(new Channel(loop, sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , highWaterMark_(64*1024*1024)  // 64MB
{
    // 下面是给channel设置的相应的回调函数，当poller给channel通知感兴趣的事件发生了之后，channel就会调用相应的操作函数
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleClose, this));
    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}


TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d \n", name_.c_str(), channel_->fd(), state_);
}

void TcpConnection::handleRead(TimeStamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), savedErrno);
    if(n > 0)
    {
        // 已经建立连接的用户， 有可读事件发生了，调用用户传入的回调操作onMessage
        messageCallback_(std::shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if(n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    int savedErrno = 0;
    ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
    if(n > 0)
    {
        outputBuffer_.retrieve(n);
        if(outputBuffer_.readableBytes() == 0)
        {
            // 用来移除写事件
            channel_->disableWriting();
            if(writeCompleteCallback_)
            {
                loop_->queueInLoop(std::bind(writeCompleteCallback_, std::shared_from_this()));
            }
        }
    }
}

void TcpConnection::handleClose()
{

}

void TcpConnection::handleError()
{

}