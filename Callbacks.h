#pragma once 

#include <memory>
#include <functional>

class Buffer;
class TcpConnection;
class TimeStamp;


using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(cosnt TcpConnectionPtr&)>;
using CloseCallback = std::function<void(TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(TcpConnectionPtr&)>;

using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*, TimeStamp)>;