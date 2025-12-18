#include "EventLoop.h"
#include "InetAddress.h"
#include "http/HttpServer.h"
#include "Logger.h"

#include <iostream>

// 一个简单的 HTTP 服务器示例，支持 GET / 和 POST /echo

void defaultHttpCallback(const HttpRequest& req, HttpResponse* resp)
{
    if (req.method() == HttpRequest::kGet && req.path() == "/")
    {
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setContentType("text/html; charset=utf-8");
        std::string body = "<html><head><title>muduo_mine</title></head>"
                           "<body><h1>Hello, muduo_mine HTTP server</h1></body></html>";
        resp->setBody(body);
    }
    else if (req.method() == HttpRequest::kPost && req.path() == "/echo")
    {
        resp->setStatusCode(HttpResponse::k200Ok);
        resp->setContentType("text/plain; charset=utf-8");
        resp->setBody(req.body());
    }
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
    InetAddress listenAddr(8080); // 监听 8080 端口
    HttpServer server(&loop, listenAddr, "HttpServer");
    server.setThreadNum(4);
    server.setHttpCallback(defaultHttpCallback);
    server.start();

    loop.loop();
    return 0;
}


