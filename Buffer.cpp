#include "Buffer.h"
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>


/*
* 从fd上读数据，Poller工作在LT模式
* Buffer缓冲区是有大小的，但是从fd读取数据时，不知道tcp数据的大小
*/

ssize_t Buffer::readFd(int fd, int* saveErrno)
{
    char extrabuf[65536] = 0; // 栈上的内存 64k的空间

    struct iovec vec[2];

    const size_t writable = writableBytes(); // 缓冲区剩余的可写空间大小
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);
    const int iovcnt = (writable < sizeof(extrabuf) ? 2 : 1);
    // readv会分散的把数据读取到vec中的缓冲区中  效率很高
    // 一般来说 vec[0]为main buf  vec[1]是临时存储区 后续再加到main buf即可 为了避免系统使用read多次读
    const ssize_t n = ::readv(fd, &vec, iovcnt);
    if(n < 0)
    {
        *saveErrno = errno;
    }
    else if(n <= writable) // Buffer的可写缓冲区已经够写所读取的数据， 不需要扩容
    {
        writerIndex_ += n;
    }
    // 超出缓冲区的数据
    else
    {
        writable = buffer_.size();
        append(extrabuf, n - writable); // extrabuf也写入了超出的数据
    }
    return n;
}


ssize_t Buffer::writeFd(int fd, int* saveErrno)
{   
    ssize_t n = ::write(fd(), peek(), readableBytes());
    if(n < 0)
    {
        // 这里的*saveErrno会修改这个函数外的saveErrno的值，所以不用返回
        *saveErrno = errno;
    }
    return n;
} 