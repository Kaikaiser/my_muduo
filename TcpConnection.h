#pragma once 
#include "noncopyable.h"

// 用于 mainloop通过acceptor获取新链接，将相应的socket，channel封装到TcpConnection中 再用轮询算法打包给subloop
class TcpConnection : noncopyable
{
public:
    
private:

};