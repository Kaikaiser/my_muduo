#pragma once

#include "TcpServer.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"

#include <functional>

// 基于 TcpServer 封装的一个简单 HTTP 服务器
// 负责：接收 TCP 连接、读写数据、解析 HTTP 请求、调用业务回调、发送 HTTP 响应
class HttpServer
{
public:
    // HTTP 业务回调类型：
    // 输入解析好的 HttpRequest，输出需要填充的 HttpResponse
    using HttpCallback = std::function<void(const HttpRequest&, HttpResponse*)>;

    HttpServer(EventLoop* loop,
               const InetAddress& listenAddr,
               const std::string& name);

    // 设置上层业务回调，用户在回调里决定如何回复
    void setHttpCallback(const HttpCallback& cb)
    {
        httpCallback_ = cb;
    }

    // 设置底层 IO 线程数量（交给 TcpServer 处理）
    void setThreadNum(int numThreads)
    {
        server_.setThreadNum(numThreads);
    }

    // 启动 HTTP 服务器：实际调用内部的 TcpServer::start()
    void start();

private:
    // 有新连接建立 / 连接关闭时的回调
    void onConnection(const TcpConnectionPtr& conn);

    // 收到数据的回调：在这里读取 Buffer、解析 HTTP 请求并生成响应
    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, TimeStamp receiveTime);

    // 将一段原始 HTTP 报文 data 解析为 HttpRequest 对象
    // 这里做的解析较简单：假设一次 read 就读到一个完整请求，不支持流水线/分块
    bool parseRequest(const std::string& data, HttpRequest* request);

    TcpServer server_;          // 底层 TCP 服务器
    HttpCallback httpCallback_; // 上层业务回调
};



