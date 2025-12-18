#include "EventLoop.h"
#include "InetAddress.h"
#include "http/HttpServer.h"
#include "Logger.h"

#include <iostream>

// 一个简单的 HTTP 服务器示例：
// - GET  /      返回一段 HTML 页面
// - POST /echo  把请求 body 原样回显
void defaultHttpCallback(const HttpRequest& req, HttpResponse* resp)
{
    // 处理 GET /
    if (req.method() == HttpRequest::kGet && req.path() == "/")
    {
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setContentType("text/html; charset=utf-8");
        std::string body =
            "<html><head><title>muduo_mine</title></head>"
            "<body><h1>Hello, muduo_mine HTTP server</h1></body></html>";
        resp->setBody(body);
    }
    // 处理 POST /echo
    else if (req.method() == HttpRequest::kPost && req.path() == "/echo")
    {
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setContentType("text/plain; charset=utf-8");
        // 将请求体直接回显给客户端
        resp->setBody(req.body());
    }
    // 其他路径 / 方法返回 404
    else
    {
        resp->setStatusCode(HttpResponse::k404NotFound);
        resp->setContentType("text/plain; charset=utf-8");
        resp->setBody("404 Not Found\r\n");
    }
}

int main()
{
    LOG_INFO("start http server...\n");

    EventLoop loop;
    InetAddress listenAddr(8080);        // 监听 8080 端口
    HttpServer server(&loop, listenAddr, "HttpServer");
    server.setThreadNum(4);              // 使用 4 个 IO 线程
    server.setHttpCallback(defaultHttpCallback); // 设置 HTTP 业务回调
    server.start();                      // 启动服务器

    // 进入事件循环，开始处理连接和请求
    loop.loop();
    return 0;
}

