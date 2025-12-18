#include "http/HttpServer.h"
#include "Logger.h"

#include <sstream>

HttpServer::HttpServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       const std::string& name)
    : server_(loop, listenAddr, name)
    , httpCallback_()
{
    server_.setConnectionCallback(
        std::bind(&HttpServer::onConnection, this, std::placeholders::_1));
    server_.setMessageCallback(
        std::bind(&HttpServer::onMessage, this,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  std::placeholders::_3));
}

void HttpServer::start()
{
    server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        LOG_INFO("HttpServer - new connection [%s] from %s\n",
                 conn->name().c_str(),
                 conn->peerAddr().toIpPort().c_str());
    }
    else
    {
        LOG_INFO("HttpServer - connection [%s] is down\n",
                 conn->name().c_str());
    }
}

void HttpServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           TimeStamp receiveTime)
{
    (void)receiveTime;
    std::string data = buf->retrieveAllAsString();

    HttpRequest request;
    if (!parseRequest(data, &request))
    {
        HttpResponse response(true);
        response.setStatusCode(HttpResponse::k400BadRequest);
        response.setContentType("text/plain; charset=utf-8");
        response.setBody("Bad Request\r\n");
        conn->send(response.toString());
        conn->shutdown();
        return;
    }

    HttpResponse response(true);
    if (httpCallback_)
    {
        httpCallback_(request, &response);
    }
    else
    {
        // 默认 404
        response.setStatusCode(HttpResponse::k404NotFound);
        response.setContentType("text/plain; charset=utf-8");
        response.setBody("404 Not Found\r\n");
    }

    std::string respStr = response.toString();
    conn->send(respStr);
    if (response.closeConnection())
    {
        conn->shutdown();
    }
}

bool HttpServer::parseRequest(const std::string& data, HttpRequest* request)
{
    // 非严格 HTTP/1.1 解析：只支持单次完整请求（不处理流水线和分块）
    const char* start = data.c_str();
    const char* end = start + data.size();

    // 找到请求行结束位置
    const char* crlf = std::search(start, end, "\r\n", "\r\n" + 2);
    if (crlf == end)
    {
        return false;
    }

    // 解析请求行：METHOD SP PATH[?query] SP HTTP/1.x
    const char* methodEnd = std::find(start, crlf, ' ');
    if (!request->setMethod(start, methodEnd))
    {
        return false;
    }
    const char* pathStart = methodEnd + 1;
    const char* pathEnd = std::find(pathStart, crlf, ' ');
    if (pathEnd == crlf)
    {
        return false;
    }

    // 拆 path 和 query
    const char* queryStart = std::find(pathStart, pathEnd, '?');
    if (queryStart != pathEnd)
    {
        request->setPath(pathStart, queryStart);
        request->setQuery(queryStart + 1, pathEnd);
    }
    else
    {
        request->setPath(pathStart, pathEnd);
    }

    // 解析 HTTP 版本
    const char* versionStart = pathEnd + 1;
    if (crlf - versionStart == 8 && std::equal(versionStart, crlf, "HTTP/1.1"))
    {
        request->setVersion(HttpRequest::kHttp11);
    }
    else if (crlf - versionStart == 8 && std::equal(versionStart, crlf, "HTTP/1.0"))
    {
        request->setVersion(HttpRequest::kHttp10);
    }
    else
    {
        request->setVersion(HttpRequest::kUnknown);
    }

    // 解析头部
    const char* headerStart = crlf + 2;
    const char* headerEnd = nullptr;
    while (headerStart < end)
    {
        headerEnd = std::search(headerStart, end, "\r\n", "\r\n" + 2);
        if (headerEnd == headerStart)
        {
            // 空行，头结束
            headerStart = headerEnd + 2;
            break;
        }
        const char* colon = std::find(headerStart, headerEnd, ':');
        if (colon != headerEnd)
        {
            request->addHeader(headerStart, colon, headerEnd);
        }
        headerStart = headerEnd + 2;
    }

    // 剩余的是 body
    if (headerStart < end)
    {
        request->setBody(std::string(headerStart, end));
    }

    return true;
}


