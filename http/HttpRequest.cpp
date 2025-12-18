#include "HttpRequest.h"

#include <algorithm>

using std::string;

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

void HttpRequest::addHeader(const char* start, const char* colon, const char* end)
{
    string field(start, colon);
    ++colon;
    while (colon < end && (*colon == ' ' || *colon == '\t'))
    {
        ++colon;
    }
    string value(colon, end);
    // 去掉末尾空白
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r' || value.back() == '\n'))
    {
        value.pop_back();
    }
    headers_[field] = value;
}

string HttpRequest::getHeader(const string& field) const
{
    auto it = headers_.find(field);
    if (it != headers_.end())
    {
        return it->second;
    }
    return "";
}


