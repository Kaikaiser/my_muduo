#pragma once

#include "TcpServer.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"

#include <functional>

// 基于 TcpServer 的简单 HTTP 服务器
class HttpServer
{
public:
    using HttpCallback = std::function<void(const HttpRequest&, HttpResponse*)>;

    HttpServer(EventLoop* loop,
               const InetAddress& listenAddr,
               const std::string& name);

    void setHttpCallback(const HttpCallback& cb)
    {
        httpCallback_ = cb;
    }

    // 设置底层 IO 线程数量
    void setThreadNum(int numThreads)
    {
        server_.setThreadNum(numThreads);
    }

    void start();

private:
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, TimeStamp receiveTime);

    // 简单一次性解析 HTTP 请求（假设一个 Buffer 中就是一个完整请求）
    bool parseRequest(const std::string& data, HttpRequest* request);

    TcpServer server_;
    HttpCallback httpCallback_;
};


