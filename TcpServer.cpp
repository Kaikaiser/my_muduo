#include "TcpServer.h"
#include "Logger.h"

EventLoop* CheckLoopNotNull(EventLoop *loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s: %s: %d mainloop does not exist! \n", __FILE__, __func__, __LINE__);
    }
    return loop;
}


TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, Option option = kNOReusePort)
    : loop_(CheckLoopNotNull(loop))
    , inPort_(listenAddr.toIpPort())

~TcpServer();