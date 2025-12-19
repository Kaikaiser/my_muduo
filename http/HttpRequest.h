#pragma once

#include <string>
#include <map>

// HttpRequest 表示一次 HTTP 请求解析后的结果
// 只负责保存「方法、版本、路径、查询字符串、头部、body」这些字段
class HttpRequest
{
public:
    // HTTP 方法枚举
    enum Method
    {
        kInvalid,   // 非法 / 未知方法
        kGet,
        kPost,
        kHead,
        kPut,
        kDelete
    };

    // HTTP 协议版本
    enum Version
    {
        kUnknown,
        kHttp10,
        kHttp11
    };

    HttpRequest()
        : method_(kInvalid)
        , version_(kUnknown)
    {}

    // 设置 HTTP 版本，例如 HTTP/1.0 或 HTTP/1.1
    void setVersion(Version v) { version_ = v; }
    Version version() const { return version_; }

    // 根据请求行中的方法字符串设置 method_
    // 返回 true 表示方法合法
    bool setMethod(const char* start, const char* end);
    Method method() const { return method_; }

    // 设置 URL 路径（不包含 ? 之后的查询参数）
    void setPath(const char* start, const char* end)
    {
        path_.assign(start, end);
    }

    const std::string& path() const { return path_; }

    // 设置查询字符串（? 后面的部分，未再拆 key=value）
    void setQuery(const char* start, const char* end)
    {
        query_.assign(start, end);
    }

    const std::string& query() const { return query_; }

    // 增加一个 HTTP 头部字段：field: value
    // 参数为在原始报文中的指针范围
    void addHeader(const char* start, const char* colon, const char* end);

    // 获取某个头字段的值，若不存在返回空字符串
    std::string getHeader(const std::string& field) const;

    // 获取所有头字段
    const std::map<std::string, std::string>& headers() const { return headers_; }

    // 设置 body（例如 POST 的内容）
    void setBody(const std::string& body) { body_ = body; }
    const std::string& body() const { return body_; }

private:
    Method method_;                            // 请求方法
    Version version_;                          // HTTP 版本
    std::string path_;                         // 路径
    std::string query_;                        // 查询字符串（原始）
    std::map<std::string, std::string> headers_; // 所有请求头
    std::string body_;                         // 请求体
};


