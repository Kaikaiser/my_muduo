# Muduo 网络库架构梳理

##  目录
1. [整体架构概述](#整体架构概述)
2. [核心模块详解](#核心模块详解)
3. [数据流和事件处理流程](#数据流和事件处理流程)
4. [线程模型](#线程模型)
5. [模块关系图](#模块关系图)

---

##  整体架构概述

Muduo 是一个基于 **Reactor 模式** 的 C++ 网络库，采用 **one loop per thread** 的线程模型。整个架构分为以下几个层次：

```
应用层 (HttpServer/TcpServer)
    ↓
连接管理层 (TcpConnection)
    ↓
事件分发层 (EventLoop + Channel)
    ↓
IO 复用层 (Poller/EPollPoller)
    ↓
系统调用层 (Socket/系统API)
```

### 核心设计思想

1. **Reactor 模式**：事件驱动，非阻塞 IO
2. **One Loop Per Thread**：每个线程一个事件循环
3. **主从 Reactor**：MainLoop 负责接受连接，SubLoop 负责处理连接
4. **线程池**：通过 EventLoopThreadPool 管理多个工作线程

---

##  核心模块详解

### 1. EventLoop（事件循环） - 核心调度器

**职责**：
- 事件循环的核心，负责调度和管理所有事件
- 管理 Channel 和 Poller，协调它们之间的交互
- 处理跨线程任务调度（通过 wakeupFd 和 pendingFunctors_）

**关键成员**：
- `poller_`：IO 多路复用器（EPollPoller）
- `wakeupFd_`：eventfd，用于唤醒其他线程的 EventLoop
- `wakeupChannel_`：监听 wakeupFd_ 的 Channel
- `activeChannels_`：当前活跃的 Channel 列表
- `pendingFunctors_`：待执行的回调函数队列

**核心流程**：
```cpp
loop() {
    while(!quit_) {
        1. poller_->poll()  // 等待事件发生
        2. 遍历 activeChannels_，调用 channel->handleEvent()
        3. doPendingFunctors()  // 执行跨线程任务
    }
}
```

**线程安全机制**：
- `runInLoop()`：如果当前在 loop 线程，直接执行；否则放入队列并唤醒
- `queueInLoop()`：将任务放入队列，通过 wakeupFd 唤醒目标线程
- `wakeup()`：向 wakeupFd 写入数据，触发 EPOLLIN 事件

---

### 2. Channel（事件通道） - 事件封装

**职责**：
- 封装一个文件描述符（fd）及其感兴趣的事件
- 保存事件回调函数（读、写、关闭、错误）
- 作为 EventLoop 和 Poller 之间的桥梁

**关键成员**：
- `fd_`：文件描述符
- `events_`：注册到 Poller 的感兴趣事件（EPOLLIN、EPOLLOUT 等）
- `revents_`：Poller 返回的实际发生的事件
- `loop_`：所属的 EventLoop
- 回调函数：`readCallback_`、`writeCallback_`、`closeCallback_`、`errorCallback_`

**事件类型**：
- `kReadEvent = EPOLLIN | EPOLLPRI`：可读事件
- `kWriteEvent = EPOLLOUT`：可写事件
- `kNoneEvent = 0`：无事件

**关键方法**：
- `enableReading()` / `disableReading()`：启用/禁用读事件
- `enableWriting()` / `disableWriting()`：启用/禁用写事件
- `handleEvent()`：当 Poller 通知事件发生时，调用相应的回调

**生命周期管理**：
- `tie()`：使用 weak_ptr 绑定 TcpConnection，防止 Channel 回调时对象已销毁

---

### 3. Poller / EPollPoller（IO 复用） - 事件监听

**职责**：
- 封装 epoll（或其他 IO 复用机制）
- 管理所有注册的 Channel
- 等待事件发生并返回活跃的 Channel 列表

**EPollPoller 关键成员**：
- `epollfd_`：epoll 文件描述符
- `events_`：epoll_event 数组，存储 epoll_wait 返回的事件
- `channels_`：fd -> Channel* 的映射表

**核心方法**：
- `poll()`：调用 `epoll_wait()`，等待事件发生，填充 activeChannels
- `updateChannel()`：调用 `epoll_ctl(EPOLL_CTL_ADD/MOD)`，注册或更新 Channel
- `removeChannel()`：调用 `epoll_ctl(EPOLL_CTL_DEL)`，移除 Channel

**Channel 状态**：
- `kNew = -1`：Channel 未添加到 Poller
- `kAdded = 1`：Channel 已添加到 Poller
- `kDeleted = 2`：Channel 已从 Poller 删除

---

### 4. TcpServer（TCP 服务器） - 服务器入口

**职责**：
- 用户编写服务器程序的主要接口
- 管理 Acceptor 和所有 TcpConnection
- 通过线程池分配连接给不同的 SubLoop

**关键成员**：
- `acceptor_`：监听新连接的 Acceptor（运行在 MainLoop）
- `threadPool_`：EventLoopThreadPool，管理多个 SubLoop
- `connections_`：所有活跃连接的映射表（name -> TcpConnectionPtr）
- 回调函数：`connectionCallback_`、`messageCallback_`、`writeCompleteCallback_`

**启动流程**：
```cpp
start() {
    1. threadPool_->start()  // 启动线程池
    2. acceptor_->listen()  // 开始监听
}
```

**新连接处理**：
```cpp
newConnection() {
    1. 通过轮询算法选择 SubLoop：ioLoop = threadPool_->getNextLoop()
    2. 创建 TcpConnection 对象
    3. 设置回调函数
    4. ioLoop->runInLoop(connectEstablished)  // 在 SubLoop 中建立连接
}
```

---

### 5. Acceptor（连接接受器） - 接受新连接

**职责**：
- 封装监听 socket（listenfd）
- 监听新连接事件，调用 accept() 接受连接
- 将新连接分发给 TcpServer

**关键成员**：
- `acceptSocket_`：监听 socket
- `acceptChannel_`：监听 socket 对应的 Channel
- `newConnectionCallback_`：新连接回调（由 TcpServer 设置）

**工作流程**：
```cpp
1. 创建非阻塞 listenfd
2. bind() + listen()
3. acceptChannel_.enableReading()  // 注册到 Poller
4. 当有新连接时，handleRead() 被调用
5. accept() 接受连接，获取 connfd
6. 调用 newConnectionCallback_(connfd, peerAddr)
```

---

### 6. TcpConnection（TCP 连接） - 连接管理

**职责**：
- 封装一个已建立的 TCP 连接
- 管理连接的读写缓冲区
- 处理连接的读写、关闭、错误事件

**关键成员**：
- `socket_`：Socket 对象（封装 connfd）
- `channel_`：连接对应的 Channel
- `inputBuffer_`：接收缓冲区
- `outputBuffer_`：发送缓冲区
- `state_`：连接状态（kConnecting、kConnected、kDisconnecting、kDisconnected）

**连接状态机**：
```
kConnecting → kConnected → kDisconnecting → kDisconnected
```

**读数据流程**：
```cpp
handleRead() {
    1. inputBuffer_.readFd(fd)  // 从 socket 读取数据到缓冲区
    2. messageCallback_(this, &inputBuffer_, receiveTime)  // 调用用户回调
}
```

**写数据流程**：
```cpp
send() {
    if (在 loop 线程) {
        sendInLoop()
    } else {
        loop_->runInLoop(sendInLoop)  // 跨线程调用
    }
}

sendInLoop() {
    1. 尝试直接 write() 发送
    2. 如果未全部发送，剩余数据放入 outputBuffer_
    3. channel_->enableWriting()  // 注册写事件
    4. 当 socket 可写时，handleWrite() 继续发送
}
```

**高水位回调**：
- 当 `outputBuffer_` 超过 `highWaterMark_` 时，触发 `highWaterMarkCallback_`
- 用于流量控制，防止发送过快

---

### 7. Buffer（缓冲区） - 数据缓冲

**职责**：
- 提供高效的数据缓冲区
- 支持从 socket 读取和写入数据
- 自动管理内存，支持动态扩容

**数据结构**：
```
| prependable | readable (content) | writable |
0    readerIndex_    writerIndex_    size()
```

**关键方法**：
- `readFd()`：使用 `readv()` 从 fd 读取数据（支持分散读）
- `writeFd()`：使用 `write()` 向 fd 写入数据
- `append()`：追加数据到缓冲区
- `retrieve()`：消费已读数据

**优化**：
- 使用 `readv()` 一次读取到主缓冲区和临时缓冲区，避免多次系统调用
- 支持内存复用，当 readerIndex_ 前移后，可以复用前面的空间

---

### 8. EventLoopThreadPool（事件循环线程池） - 线程管理

**职责**：
- 管理多个 EventLoop 线程（SubLoop）
- 提供轮询算法分配连接给不同的 SubLoop
- 实现主从 Reactor 模式

**关键成员**：
- `baseLoop_`：主 EventLoop（MainLoop）
- `threads_`：EventLoopThread 列表
- `loops_`：所有 SubLoop 的指针列表
- `next_`：轮询索引

**工作流程**：
```cpp
start() {
    for (i = 0; i < numThreads_; ++i) {
        1. 创建 EventLoopThread
        2. thread->startLoop()  // 启动线程，返回 EventLoop*
        3. 将 EventLoop* 加入 loops_
    }
}

getNextLoop() {
    if (loops_ 为空) {
        return baseLoop_  // 单线程模式
    }
    return loops_[next_++ % loops_.size()]  // 轮询分配
}
```

---

### 9. EventLoopThread（事件循环线程） - 线程封装

**职责**：
- 封装一个线程，该线程运行一个 EventLoop
- 提供线程安全的 EventLoop 获取接口

**工作流程**：
```cpp
threadFunc() {
    1. 创建 EventLoop
    2. 执行 threadInitCallback_(loop)
    3. loop->loop()  // 进入事件循环
    4. 返回 loop 指针
}
```

---

## 🔄 数据流和事件处理流程

### 服务器启动流程

```
1. 用户创建 EventLoop（MainLoop）
2. 创建 TcpServer，传入 MainLoop
3. TcpServer 创建 Acceptor，绑定到 MainLoop
4. TcpServer 创建 EventLoopThreadPool
5. 调用 TcpServer::start()
   ├─ threadPool_->start()  // 启动 SubLoop 线程池
   └─ acceptor_->listen()   // MainLoop 开始监听
6. MainLoop.loop() 开始事件循环
```

### 新连接建立流程

```
1. 客户端连接 → listenfd 可读
2. MainLoop 的 Poller 检测到事件
3. Acceptor::handleRead() 被调用
4. accept() 接受连接，获得 connfd
5. TcpServer::newConnection(connfd, peerAddr)
   ├─ 通过轮询选择 SubLoop
   ├─ 创建 TcpConnection 对象
   ├─ 设置回调函数
   └─ SubLoop->runInLoop(connectEstablished)
6. TcpConnection::connectEstablished()
   ├─ channel_->enableReading()  // 注册到 SubLoop 的 Poller
   └─ connectionCallback_(this)  // 通知用户
```

### 数据读取流程

```
1. 客户端发送数据 → connfd 可读
2. SubLoop 的 Poller 检测到事件
3. Channel::handleEvent() 被调用
4. TcpConnection::handleRead()
   ├─ inputBuffer_.readFd(fd)  // 读取数据到缓冲区
   └─ messageCallback_(this, &inputBuffer_, time)  // 用户处理数据
```

### 数据发送流程

```
1. 用户调用 TcpConnection::send(data)
2. 如果不在 loop 线程，通过 runInLoop() 切换到 loop 线程
3. TcpConnection::sendInLoop()
   ├─ 尝试直接 write() 发送
   ├─ 如果未全部发送，剩余数据放入 outputBuffer_
   └─ channel_->enableWriting()  // 注册写事件
4. 当 socket 可写时，Channel::handleEvent() 触发
5. TcpConnection::handleWrite()
   ├─ outputBuffer_.writeFd(fd)  // 继续发送
   ├─ 发送完成后，channel_->disableWriting()
   └─ writeCompleteCallback_(this)  // 通知用户
```

### 连接关闭流程

```
1. 客户端关闭连接 → connfd 可读（read 返回 0）
2. TcpConnection::handleRead() 检测到 n == 0
3. TcpConnection::handleClose()
   ├─ setState(kDisconnecting)
   ├─ channel_->disableAll()
   ├─ connectionCallback_(this)  // 通知连接关闭
   └─ closeCallback_(this)  // 调用 TcpServer::removeConnection
4. TcpServer::removeConnection()
   ├─ 从 connections_ 中删除
   └─ SubLoop->queueInLoop(connectDestroyed)
5. TcpConnection::connectDestroyed()
   ├─ channel_->remove()  // 从 Poller 中移除
   └─ TcpConnection 对象析构
```

---

## 🧵 线程模型

### One Loop Per Thread

- **每个线程一个 EventLoop**：保证事件处理在单线程内，无需加锁
- **MainLoop（主 Reactor）**：运行在用户线程，负责接受新连接
- **SubLoop（从 Reactor）**：运行在工作线程，负责处理已建立的连接

### 线程间通信

**MainLoop → SubLoop**：
1. MainLoop 通过轮询选择 SubLoop
2. 调用 `SubLoop->runInLoop(callback)` 或 `queueInLoop(callback)`
3. 通过 `wakeupFd_` 写入数据，唤醒 SubLoop
4. SubLoop 的 `wakeupChannel_` 检测到事件，执行 `doPendingFunctors()`

**跨线程调用保证**：
- `runInLoop()`：如果当前在 loop 线程，直接执行；否则放入队列
- `queueInLoop()`：放入队列，必要时唤醒目标线程
- 所有对 TcpConnection 的操作都在其所属的 loop 线程中执行

### 线程安全

- **EventLoop**：每个 EventLoop 只在一个线程中运行，内部操作无需加锁
- **TcpConnection**：所有操作都在其所属的 loop 线程中执行
- **pendingFunctors_**：使用 mutex 保护，支持跨线程添加任务

---

##  模块关系图

```
┌─────────────────────────────────────────────────────────┐
│                    应用层                               │
│  ┌──────────────┐         ┌──────────────┐            │
│  │  TcpServer   │         │  HttpServer  │            │
│  └──────┬───────┘         └──────┬───────┘            │
└─────────┼────────────────────────┼────────────────────┘
          │                        │
          │                        │
┌─────────▼────────────────────────▼───────────────────┐
│              连接管理层                               │
│  ┌──────────────────────────────────────────────┐    │
│  │          TcpConnection                       │    │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐    │    │
│  │  │  Socket  │  │ Channel  │  │  Buffer  │    │    │
│  │  └──────────┘  └────┬─────┘  └──────────┘    │    │
│  └─────────────────────┼────────────────────────┘    │
└────────────────────────┼─────────────────────────────┘
                         │
┌────────────────────────▼─────────────────────────────┐
│              事件分发层                               │
│  ┌──────────────────────────────────────────────┐    │
│  │            EventLoop                         │    │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐    │    │
│  │  │ Channel  │  │  Poller  │  │ wakeupFd │    │    │
│  │  │   List   │  │          │  │ Channel  │    │    │
│  │  └──────────┘  └────┬─────┘  └──────────┘    │    │
│  └─────────────────────┼────────────────────────┘    │
└────────────────────────┼─────────────────────────────┘
                         │
┌────────────────────────▼─────────────────────────────┐
│                    IO 复用层                          │
│  ┌──────────────────────────────────────────────┐    │
│  │         EPollPoller                          │    │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐    │    │
│  │  │ epollfd  │  │ Channel  │  │ epoll_   │    │    │
│  │  │          │  │   Map    │  │  wait    │    │    │
│  │  └──────────┘  └──────────┘  └──────────┘    │    │
│  └──────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────┘
                         │
┌────────────────────────▼─────────────────────────────┐
│              系统调用层                               │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐            │
│  │  socket  │  │  bind    │  │  listen  │            │
│  │  accept  │  │  read    │  │  write   │            │
│  │  epoll_  │  │  epoll_  │  │  epoll_  │            │
│  │  create  │  │  ctl     │  │  wait    │            │
│  └──────────┘  └──────────┘  └──────────┘            │
└──────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────┐
│              线程管理                                 │
│  ┌──────────────────────────────────────────────┐    │
│  │      EventLoopThreadPool                     │    │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐    │    │
│  │  │MainLoop  │  │SubLoop1  │  │SubLoop2  │... │    │
│  │  └──────────┘  └──────────┘  └──────────┘    │    │
│  └──────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────┘
```

---

##  关键设计模式

### 1. Reactor 模式
- **事件驱动**：所有操作都是事件驱动的
- **非阻塞 IO**：使用 epoll 实现高效的 IO 复用
- **回调机制**：通过回调函数处理各种事件

### 2. 主从 Reactor 模式
- **MainLoop**：接受新连接（Acceptor）
- **SubLoop**：处理已建立的连接（TcpConnection）

### 3. 对象生命周期管理
- **智能指针**：使用 `shared_ptr` 管理 TcpConnection
- **weak_ptr**：Channel 使用 `weak_ptr` 绑定 TcpConnection，避免循环引用

### 4. 缓冲区设计
- **应用层缓冲区**：避免频繁系统调用
- **高水位回调**：流量控制机制

---

##  总结

Muduo 网络库的核心优势：

1. **高性能**：基于 epoll 的非阻塞 IO，主从 Reactor 模式
2. **线程安全**：One Loop Per Thread，避免锁竞争
3. **易用性**：清晰的接口设计，回调机制灵活
4. **可扩展性**：模块化设计，易于扩展新功能

整个架构通过 **EventLoop** 作为核心调度器，**Channel** 作为事件封装，**Poller** 作为 IO 复用，实现了高效、可扩展的网络编程框架。

