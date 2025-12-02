#pragma once 
#include "noncopyable.h"
#include "Callbacks.h"
#include "InetAddress.h"
#include "Buffer.h"
#include <memory>
#include <string>
#include <atomic>


class Channel;
class EventLoop;
class Socket;


/**
 * TcpSever => Acceptor => 有一个用户连接，通过accept函数拿到connfd
 * => TcpConnection 设置回调 
*/


// 用于 mainloop通过acceptor获取新链接，将相应的socket，channel封装到TcpConnection中 再用轮询算法打包给subloop
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop,
                const std::string &name,
                int sockfd,
                const InetAddress &localAddr,
                const InetAddress &peerAddr);

    ~TcpConnection();

    EventLoop* getloop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddr() const { return localAddr_; }
    const InetAddress& peerAddr() const { return peerAddr_; }
    bool connected () const { return state_ == kConnected; }
    
    // 发送数据
    void send(const void *message, int len);
    // 关闭连接
    void shutdown();
private:
    enum StateE {kDisconnected, kDisconnecting, kConnected, kConnecting};
    EventLoop *loop_; // 这里不是baseloop 因为TcpConnection是在subloop中进行管理的
    const std::string name_;
    std::atomic_int state_;
    bool reading_;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    const InetAddress localAddr_;
    const InetAddress peerAddr_;
    ConnectionCallback connectionCallback_;  // 有新连接时的回调
    MessageCallback messageCallback_; // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完成后的回调
    HighWaterMarkCallback highWaterMarkCallback_; // 高水位回调
    CloseCallback closeCallback_; 
    size_t highWaterMark_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;


};