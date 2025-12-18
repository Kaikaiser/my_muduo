#pragma once

#include <string>
#include <map>

// HttpResponse 表示要发送给客户端的一次 HTTP 响应
// 通过设置状态码、头部和 body，最后调用 toString() 生成完整报文
class HttpResponse
{
public:
    // HTTP 状态码枚举
    enum HttpStatusCode
    {
        kUnknown,
        k200Ok = 200,
        k400BadRequest = 400,
        k404NotFound = 404,
        k500ServerError = 500
    };

    // close 为 true 表示响应后主动关闭连接（Connection: close）
    explicit HttpResponse(bool close = true)
        : statusCode_(kUnknown)
        , closeConnection_(close)
    {}

    // 设置 HTTP 状态码，如 200、404
    void setStatusCode(HttpStatusCode code) { statusCode_ = code; }

    // 可显式设置状态描述，如 "OK"，不设置时会根据 code 自动匹配
    void setStatusMessage(const std::string& message) { statusMessage_ = message; }

    // 设置是否在响应后关闭连接
    void setCloseConnection(bool on) { closeConnection_ = on; }
    bool closeConnection() const { return closeConnection_; }

    // 设置 Content-Type 头，例如 "text/html; charset=utf-8"
    void setContentType(const std::string& type)
    {
        addHeader("Content-Type", type);
    }

    // 增加 / 修改一个通用的响应头
    void addHeader(const std::string& key, const std::string& value)
    {
        headers_[key] = value;
    }

    // 设置响应 body
    void setBody(const std::string& body)
    {
        body_ = body;
    }

    // 生成完整 HTTP 响应报文字符串：
    // 状态行 + 头部 + 空行 + body
    std::string toString() const;

private:
    HttpStatusCode statusCode_;                // 状态码
    std::string statusMessage_;                // 状态文本（可选）
    std::map<std::string, std::string> headers_; // 头部字段
    std::string body_;                         // 响应体
    bool closeConnection_;                     // 是否关闭连接
};

