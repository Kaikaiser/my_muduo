#include "http/HttpServer.h"
#include "Logger.h"

#include <sstream>

HttpServer::HttpServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       const std::string& name)
    : server_(loop, listenAddr, name)
    , httpCallback_()
{
    // 设置 TCP 连接建立 / 断开 时的回调
    server_.setConnectionCallback(
        std::bind(&HttpServer::onConnection, this, std::placeholders::_1));

    // 设置收到数据时的回调
    server_.setMessageCallback(
        std::bind(&HttpServer::onMessage, this,
                  std::placeholders::_1,
                  std::placeholders::_2,
                  std::placeholders::_3));
}

// 启动底层 TcpServer，开始监听端口并处理事件
void HttpServer::start()
{
    server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        // 新连接建立
        LOG_INFO("HttpServer - new connection [%s] from %s\n",
                 conn->name().c_str(),
                 conn->peerAddr().toIpPort().c_str());
    }
    else
    {
        // 连接断开
        LOG_INFO("HttpServer - connection [%s] is down\n",
                 conn->name().c_str());
    }
}

void HttpServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           TimeStamp receiveTime)
{
    (void)receiveTime; // 目前未使用时间戳，避免未使用参数告警

    // 一次性取出 Buffer 中的所有数据，假设就是一个完整的 HTTP 请求报文
    std::string data = buf->retrieveAllAsString();

    HttpRequest request;
    // 解析 HTTP 请求失败，直接返回 400
    if (!parseRequest(data, &request))
    {
        HttpResponse response(true); // 关闭连接
        response.setStatusCode(HttpResponse::k400BadRequest);
        response.setContentType("text/plain; charset=utf-8");
        response.setBody("Bad Request\r\n");
        conn->send(response.toString());
        conn->shutdown();
        return;
    }

    // 解析成功，交给业务回调处理
    HttpResponse response(true); // 默认响应后关闭连接
    if (httpCallback_)
    {
        httpCallback_(request, &response);
    }
    else
    {
        // 若业务层没有设置回调，则返回一个简单的 404
        response.setStatusCode(HttpResponse::k404NotFound);
        response.setContentType("text/plain; charset=utf-8");
        response.setBody("404 Not Found\r\n");
    }

    // 序列化为字符串并发送
    std::string respStr = response.toString();
    conn->send(respStr);

    // 根据 response 的连接策略决定是否关闭 TCP 连接
    if (response.closeConnection())
    {
        conn->shutdown();
    }
}

// 一个非常简化版的 HTTP/1.x 请求解析器
// 只处理：请求行 + 若干头部 + body，不考虑分块传输和多请求流水线
bool HttpServer::parseRequest(const std::string& data, HttpRequest* request)
{
    const char* start = data.c_str();
    const char* end = start + data.size();

    // 查找第一行（请求行）的 \r\n 结束位置
    const char* crlf = std::search(start, end, "\r\n", "\r\n" + 2);
    if (crlf == end)
    {
        return false;
    }

    // 请求行格式：METHOD SP PATH[?query] SP HTTP/1.x
    const char* methodEnd = std::find(start, crlf, ' ');
    if (!request->setMethod(start, methodEnd))
    {
        return false;
    }

    const char* pathStart = methodEnd + 1;
    const char* pathEnd = std::find(pathStart, crlf, ' ');
    if (pathEnd == crlf)
    {
        // 没有找到第二个空格，非法请求行
        return false;
    }

    // 拆分 path 和 query
    const char* queryStart = std::find(pathStart, pathEnd, '?');
    if (queryStart != pathEnd)
    {
        // 形如 /path?query
        request->setPath(pathStart, queryStart);
        request->setQuery(queryStart + 1, pathEnd);
    }
    else
    {
        // 只有 /path
        request->setPath(pathStart, pathEnd);
    }

    // 解析 HTTP 版本，例如 "HTTP/1.1"
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

    // 解析头部：从请求行后面的下一行开始
    const char* headerStart = crlf + 2;
    const char* headerEnd = nullptr;
    while (headerStart < end)
    {
        headerEnd = std::search(headerStart, end, "\r\n", "\r\n" + 2);
        if (headerEnd == headerStart)
        {
            // 遇到空行，说明头部结束，接下来是 body
            headerStart = headerEnd + 2;
            break;
        }
        const char* colon = std::find(headerStart, headerEnd, ':');
        if (colon != headerEnd)
        {
            // 形如 "Host: 127.0.0.1"
            request->addHeader(headerStart, colon, headerEnd);
        }
        headerStart = headerEnd + 2;
    }

    // headerStart 之后的内容全部视为 body（未按 Content-Length 再校验）
    if (headerStart < end)
    {
        request->setBody(std::string(headerStart, end));
    }

    return true;
}


