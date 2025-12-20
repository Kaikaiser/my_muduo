#pragma once 
#include <vector>
#include <string>
#include <algorithm>
// 网络库底层的缓冲区类型的定义
// --------------------------------------------------------------
// |  prependable bytes   |  readable bytes  |  writable bytes  |         
// |                      |     (content)    |                  |
// --------------------------------------------------------------
// 0                 readerIndex_         writerIndex_     buffer.size() 

class Buffer
{
public:
    static const size_t kCheapPrepend = 0;
    static const size_t kInitialSize = 1024;
    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend)
    {}

    size_t readableBytes() const 
    {
        return writerIndex_ - readerIndex_;
    }

    size_t writableBytes() const 
    {
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const 
    {
        return readerIndex_;
    }
    // 读取缓冲区中可读取的数据的地址
    const char* peek() const 
    {
        return begin() + readerIndex_;
    }

    // onMessage  Buffer -> string 
    void retrieve(size_t len)
    {
        if(len < readableBytes())
        {
            readerIndex_ += len; // 应用只读取了刻度缓冲区数据的一部分，就是len，还剩下writerIndex_ - readerIndex_ += len 大小没读
        }
        else // len == readableBytes() 读完了已经
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    // 把onMessage上报的Buffer函数 转换成string类型的数据返回
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());  // 应用可读取数据的长度
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len); // 上面一句把缓冲区中可读的数据已经读取出来了，这里肯定要对缓冲区进行复位操作
        return result;
    }

    // buffer_.size() - writerIndex_  
    void ensureWriteableBytes(size_t len)
    {
        if(writableBytes() < len)
        {
            makeSpace(len); // 扩容函数
        }
    }
    // 把[data, data + lem]上的数据添加到writable缓冲区中
    void append(const char* data, size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    char* beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char* beginWrite() const
    {
        return begin() + writerIndex_;
    }

    // 从fd上读数据
    ssize_t readFd(int fd, int* saveErrno);
    // 从fd上写数据
    ssize_t writeFd(int fd, int* saveErrno);
private:
    char* begin()
    {
        // begin()是返回首元素的迭代器  it.operator*()重载获取起始元素 然后再&获取起始地址
        return &*buffer_.begin(); // vector底层数组的起始元素的地址 也就是数组的其实地址
    }
    const char* begin() const 
    {
        return &*buffer_.begin();
    }
    void makeSpace(size_t len)
    {
        if(writableBytes() + prependableBytes() < kCheapPrepend + len)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readableBytes();
        }
    }
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

};