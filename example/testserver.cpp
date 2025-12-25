#include <muduomy/TcpServer.h>
#include <muduomy/Logger.h>

#include <functional>
#include <string>

class EchoServer
{
public:
    EchoServer(EventLoop *loop, const InetAddress &addr, const std::string &name)
        : server_(loop, addr, name)
        , loop_(loop)
    {
        // 注册回调函数
        server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, std::placeholders::_1));

        server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, std::placeholders::_1, 
            std::placeholders::_2, std::placeholders::_3));

        // 设定线程数量（不包括baseloop）  一般来说 线程数 = 核数
        server_.setThreadNum(3);  // 一个io（主）  三个worker（sub）
    }

    // 启动TcpServer
    void start()
    {
        server_.start();
    }

private:
    // 连接建立或者断开的回调
    void onConnection(const TcpConnectionPtr &conn)
    {
        if(conn->connected())
        {
            LOG_INFO("Connection UP : %s", conn->peerAddr().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("Connection DOWN : %s", conn->peerAddr().toIpPort().c_str());
        }
    }

    // 可读写事件的回调
    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, TimeStamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        conn->shutdown(); // 关闭写端  EPOLLHUP -> closeCallback 
    }
    EventLoop* loop_;
    TcpServer server_;
};


// 主逻辑
int main()
{
    EventLoop loop; // mainloop
    InetAddress addr(8000);
    EchoServer server(&loop, addr, "EchoServer_test"); // 初始化TcpServer创建Acceptor -> non-blocking listenfd  create bind 
    server.start(); // listen loopthread listenfd -> acceptChannel ->mainloop -> poller
    loop.loop(); // 启动mainloop的事件循环以及底层Poller

    return 0;
}