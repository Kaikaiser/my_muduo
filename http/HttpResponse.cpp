#include "HttpResponse.h"

#include <sstream>

// 将当前 HttpResponse 对象序列化为 HTTP/1.1 文本报文
std::string HttpResponse::toString() const
{
    std::ostringstream oss;
    // 状态行
    int code = static_cast<int>(statusCode_);
    std::string message = statusMessage_;

    // 如果用户没手动设置 statusMessage_，根据状态码给一个默认文案
    if (message.empty())
    {
        switch (statusCode_)
        {
        case k200Ok:         message = "OK"; break;
        case k400BadRequest: message = "Bad Request"; break;
        case k404NotFound:   message = "Not Found"; break;
        case k500ServerError:message = "Internal Server Error"; break;
        default:             message = "Unknown";
        }
    }

    // 示例：HTTP/1.1 200 OK\r\n
    oss << "HTTP/1.1 " << code << " " << message << "\r\n";

    // 输出所有用户设置的头部
    for (const auto& header : headers_)
    {
        oss << header.first << ": " << header.second << "\r\n";
    }

    // 自动添加 Content-Length 头
    oss << "Content-Length: " << body_.size() << "\r\n";

    // 根据 closeConnection_ 决定 Connection 头为 close 或 keep-alive
    if (closeConnection_)
    {
        oss << "Connection: close\r\n";
    }
    else
    {
        oss << "Connection: keep-alive\r\n";
    }

    // 头部和 body 之间的空行
    oss << "\r\n";

    // body 内容
    oss << body_;
    return oss.str();
}

