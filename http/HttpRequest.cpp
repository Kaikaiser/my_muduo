#include "HttpRequest.h"

#include <algorithm>

using std::string;

// 解析请求方法字符串，设置内部枚举值
bool HttpRequest::setMethod(const char* start, const char* end)
{
    string m(start, end);
    if (m == "GET")
    {
        method_ = kGet;
    }
    else if (m == "POST")
    {
        method_ = kPost;
    }
    else if (m == "HEAD")
    {
        method_ = kHead;
    }
    else if (m == "PUT")
    {
        method_ = kPut;
    }
    else if (m == "DELETE")
    {
        method_ = kDelete;
    }
    else
    {
        method_ = kInvalid;
    }
    return method_ != kInvalid;
}

// 将一行 "Field: value\r\n" 拆成 key/value 存入 map
void HttpRequest::addHeader(const char* start, const char* colon, const char* end)
{
    // header 名称，如 "Content-Type"
    string field(start, colon);
    ++colon;
    // 跳过冒号后面的空格或制表符
    while (colon < end && (*colon == ' ' || *colon == '\t'))
    {
        ++colon;
    }
    // header 值的原始字符串
    string value(colon, end);
    // 去掉末尾的空白字符（空格、Tab、回车、换行）
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r' || value.back() == '\n'))
    {
        value.pop_back();
    }
    headers_[field] = value;
}

// 根据字段名从 map 中查找 header 值
string HttpRequest::getHeader(const string& field) const
{
    auto it = headers_.find(field);
    if (it != headers_.end())
    {
        return it->second;
    }
    return "";
}

