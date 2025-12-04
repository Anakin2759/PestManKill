# 服务器端 NetWorkManager 使用说明

## 概述

服务器端的 `NetWorkManager` 是一个基于 ASIO + C++23 协程的 UDP 网络管理器，提供以下特性：

- 完全由 GameContext 中的 ThreadPool 驱动
- 支持异步发送/接收
- 支持可靠传输（ACK机制，事件驱动）
- 服务器模式（可同时管理多个客户端连接）

## 基本使用

### 1. 创建 NetWorkManager

```cpp
#include "src/server/net/NetWorkManager.h"
#include "src/server/context/GameContext.h"

GameContext context;
auto networkManager = std::make_shared<NetWorkManager>(context.threadPool, context.logger);
```

### 2. 启动服务器

```cpp
uint16_t port = 8888;
networkManager->start(port);
```

### 3. 设置数据包处理器

```cpp
networkManager->setPacketHandler(
    [](const uint8_t* data, size_t size, const asio::ip::udp::endpoint& sender) {
        // 处理接收到的数据包
        // data: 数据包内容（不包含 PacketHeader）
        // size: 数据大小
        // sender: 发送方的 endpoint
    }
);
```

### 4. 发送数据

#### 不可靠发送（UDP）

```cpp
asio::ip::udp::endpoint clientEndpoint = /* 客户端地址 */;
std::vector<uint8_t> payload = {/* 数据内容 */};
uint16_t messageType = 0x0001;

asio::co_spawn(
    context.threadPool.get_executor(),
    [networkManager, clientEndpoint, messageType, payload]() -> asio::awaitable<void> {
        co_await networkManager->sendPacket(clientEndpoint, messageType, payload);
    },
    asio::detached
);
```

#### 可靠发送（带 ACK 确认）

```cpp
asio::co_spawn(
    context.threadPool.get_executor(),
    [networkManager, clientEndpoint, messageType, payload]() -> asio::awaitable<void> {
        bool success = co_await networkManager->sendReliablePacket(
            clientEndpoint,
            messageType, 
            payload,
            3,  // 最多重试3次
            std::chrono::milliseconds(1000)  // 超时时间
        );
        
        if (success) {
            // 发送成功并收到 ACK
        } else {
            // 发送失败或未收到 ACK
        }
    },
    asio::detached
);
```

### 5. 停止服务器

```cpp
networkManager->stop();
```

## 与客户端版本的区别

| 特性 | 客户端 | 服务器 |
|------|--------|--------|
| 初始化方式 | 无参构造 | 需要传入 threadPool 和 logger |
| 连接方式 | connect(host, port) | start(port) 监听 |
| 发送目标 | 固定的服务器 endpoint | 需要指定客户端 endpoint |
| ACK 管理 | 按 seq 区分 | 按 (seq, endpoint) 区分 |
| 线程池 | 单例 ThreadPool | GameContext 中的 threadPool |

## 注意事项

1. **协程参数**: 所有协程函数的参数都不应使用引用（会导致编译错误）
2. **线程安全**: 内部使用 mutex 保护 ACK 管理，可以安全地从多个线程调用
3. **资源管理**: 使用 shared_from_this()，确保在异步操作期间对象不会被销毁
4. **日志**: 使用传入的 spdlog::logger 进行日志记录

## 示例：完整的消息处理流程

```cpp
#include "src/server/net/NetWorkManager.h"
#include "src/server/context/GameContext.h"
#include "src/shared/messages/PacketHeader.h"

void setupNetworkSystem(GameContext& context) {
    auto networkManager = std::make_shared<NetWorkManager>(
        context.threadPool, 
        context.logger
    );
    
    // 设置数据包处理器
    networkManager->setPacketHandler(
        [&context, networkManager](
            const uint8_t* data, 
            size_t size, 
            const asio::ip::udp::endpoint& sender
        ) {
            // 解析消息类型和内容
            // 根据类型分发到不同的处理逻辑
            
            // 回复客户端
            asio::co_spawn(
                context.threadPool.get_executor(),
                [networkManager, sender]() -> asio::awaitable<void> {
                    std::vector<uint8_t> response = {/* 响应数据 */};
                    co_await networkManager->sendReliablePacket(
                        sender, 
                        0x0002,  // 响应消息类型
                        response
                    );
                },
                asio::detached
            );
        }
    );
    
    // 启动服务器
    networkManager->start(8888);
    
    context.logger->info("网络系统已启动，监听端口: 8888");
}
```
