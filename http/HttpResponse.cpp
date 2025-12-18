#include "HttpResponse.h"

#include <sstream>

std::string HttpResponse::toString() const
{
    std::ostringstream oss;
    // 状态行
    int code = static_cast<int>(statusCode_);
    std::string message = statusMessage_;

    if (message.empty())
    {
        switch (statusCode_)
        {
        case k200Ok:        message = "OK"; break;
        case k400BadRequest:message = "Bad Request"; break;
        case k404NotFound:  message = "Not Found"; break;
        case k500ServerError:message = "Internal Server Error"; break;
        default:            message = "Unknown";
        }
    }

    oss << "HTTP/1.1 " << code << " " << message << "\r\n";

    // 头部
    for (const auto& header : headers_)
    {
        oss << header.first << ": " << header.second << "\r\n";
    }

    // Content-Length
    oss << "Content-Length: " << body_.size() << "\r\n";

    // 连接
    if (closeConnection_)
    {
        oss << "Connection: close\r\n";
    }
    else
    {
        oss << "Connection: keep-alive\r\n";
    }

    oss << "\r\n";

    // body
    oss << body_;
    return oss.str();
}


