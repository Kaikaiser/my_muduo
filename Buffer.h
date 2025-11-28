#pragma once 
#include <vector>
// 网络库底层的缓冲区类型的定义
class Buffer
{
public:
    static const size_t kCheapPrepend = 0;
    static const size_t kInitialSize = 1024;

private:
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

};