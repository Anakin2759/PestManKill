# Net 模块使用指南

## 概述

Net 模块是基于 **KCP + ASIO** 的可靠 UDP 网络库，提供高性能、低延迟的网络通信能力。采用 C++23 协程设计，支持服务端和客户端开发。

### 核心特性

- **传输层**: UDP + KCP 协议（可靠传输）
- **异步模型**: ASIO 协程 (`asio::awaitable`)
- **帧协议**: 自定义二进制帧格式（magic + cmd + length + payload）
- **会话管理**: 自动创建/销毁会话，超时清理
- **线程模型**: IO 线程 + 业务线程池分离

---

## 架构设计

### 分层结构

```
┌─────────────────────────────────────┐
│   Application Layer (Server/Client) │  业务逻辑层
├─────────────────────────────────────┤
│   KcpEndpoint (会话管理 + 分发)      │  端点抽象层
├─────────────────────────────────────┤
│   KcpSession (KCP 协议封装)          │  会话层
├─────────────────────────────────────┤
│   IUdpTransport (传输接口)           │  传输抽象层
├─────────────────────────────────────┤
│   AsioUdpTransport (ASIO 实现)       │  传输实现层
└─────────────────────────────────────┘
```

### 关键概念

| 概念               | 说明                                  |
| ------------------ | ------------------------------------- |
| **Conv ID**  | KCP 会话标识符，用于区分不同的连接    |
| **Endpoint** | 网络端点（IP + Port）                 |
| **Session**  | KCP 会话实例，封装一个连接的收发逻辑  |
| **Frame**    | 应用层数据帧（包含 header + payload） |

---

## 核心类与接口

### 1. 传输层接口

#### IUdpTransport

```cpp
class IUdpTransport
{
public:
    virtual ~IUdpTransport() = default;
    virtual void send(const asio::ip::udp::endpoint& to, std::span<const uint8_t> data) = 0;
};
```

**职责**: 定义 UDP 传输抽象接口

**实现类**:

- `AsioUdpTransport`: 基于 ASIO 的真实 UDP 传输
- `MockUdpTransport`: 单元测试用的模拟实现

---

#### AsioUdpTransport

```cpp
class AsioUdpTransport final : public IUdpTransport
{
public:
    AsioUdpTransport(asio::any_io_executor exec, uint16_t port);
  
    void send(const asio::ip::udp::endpoint& to, std::span<const uint8_t> data) override;
    uint16_t localPort() const;
    void stop();
  
    // 非协程接收（回调模式）
    template <typename PacketHandler>
    void startReceive(PacketHandler&& handler);
  
    // 协程接收循环
    template <typename PacketHandler>
    asio::awaitable<void> recvLoop(PacketHandler&& handler);
};
```

**使用示例**:

```cpp
asio::io_context ioc;
AsioUdpTransport transport(ioc.get_executor(), 8888);

// 方式1: 回调模式
transport.startReceive([](const auto& from, std::span<const uint8_t> data) {
    // 处理收到的数据
});

// 方式2: 协程模式
asio::co_spawn(ioc, transport.recvLoop([](const auto& from, auto data) {
    // 处理数据
}), asio::detached);

ioc.run();
```

---

### 2. 会话层

#### KcpSession

```cpp
class KcpSession
{
public:
    KcpSession(uint32_t conv, IUdpTransport& transport, 
               asio::ip::udp::endpoint peer, asio::any_io_executor exec);
  
    // 喂入底层 UDP 数据
    void input(std::span<const uint8_t> data);
  
    // 协程接口：异步接收完整 KCP 包
    asio::awaitable<std::expected<Packet, std::error_code>> recv();
  
    // 发送数据
    void send(std::span<const uint8_t> data);
  
    // 定期更新 KCP 状态
    void update(uint32_t now_ms);
    uint32_t check(uint32_t now_ms) const;
};
```

**核心特性**:

- 封装 KCP 协议，提供可靠 UDP 传输
- 使用 `asio::experimental::channel` 实现协程通信
- 自动处理 KCP 的发送/接收/重传逻辑

**使用示例**:

```cpp
// 接收数据（协程中）
auto result = co_await session->recv();
if (result) {
    const auto& packet = *result;
    // 处理数据包
}

// 发送数据
std::vector<uint8_t> data = {0x01, 0x02, 0x03};
session->send(data);
session->update(getCurrentTimeMs()); // 需定期调用
```

---

### 3. 端点抽象层

#### KcpEndpoint

```cpp
class KcpEndpoint
{
protected:
    KcpEndpoint(asio::any_io_executor exec, IUdpTransport& transport);
  
    // 处理收到的 UDP 包并分发给对应会话
    void input(const asio::ip::udp::endpoint& from, std::span<const uint8_t> data);
  
    // 更新所有会话，清理超时会话
    void update(uint32_t now_ms, std::chrono::seconds timeout = 30s);
  
    // 虚函数：子类实现具体的 Conv 选择逻辑
    virtual uint32_t selectConv(const asio::ip::udp::endpoint&, std::span<const uint8_t>) = 0;
  
    // 虚函数：会话创建回调
    virtual void onSession(uint32_t conv, std::shared_ptr<KcpSession> session);
    virtual void onSessionClosed(uint32_t conv);
  
protected:
    std::unordered_map<uint32_t, std::shared_ptr<KcpSession>> m_sessions;
};
```

**职责**:

- 管理多个 KCP 会话（一对多连接）
- 自动路由数据到对应会话
- 超时清理机制

---

### 4. 应用层实现

#### Server（服务端）

```cpp
class Server : public KcpEndpoint
{
public:
    Server(IUdpTransport& transport, asio::any_io_executor io_exec, size_t thread_count);
    void stop();
  
protected:
    uint32_t selectConv(const asio::ip::udp::endpoint&, std::span<const uint8_t> data) override;
    void onSession(uint32_t conv, std::shared_ptr<KcpSession> session) override;
  
private:
    static asio::awaitable<void> playerRoutine(uint32_t conv, std::shared_ptr<KcpSession> session);
    asio::thread_pool m_pool;
};
```

**特性**:

- 自动为每个新连接创建会话
- 将业务逻辑派发到线程池（通过 `asio::strand` 保证线程安全）
- 从数据包中提取 Conv ID（`peekConv`）

**使用示例**:

```cpp
asio::io_context ioc;
AsioUdpTransport transport(ioc.get_executor(), 9999);
Server server(transport, ioc.get_executor(), 4); // 4个工作线程

// 启动接收循环
asio::co_spawn(ioc, transport.recvLoop([&](const auto& from, auto data) {
    server.input(from, data);
}), asio::detached);

// 定时更新会话
asio::steady_timer timer(ioc);
auto updateLoop = [&]() -> asio::awaitable<void> {
    while (true) {
        co_await timer.async_wait(asio::use_awaitable);
        server.update(getCurrentTimeMs());
        timer.expires_after(std::chrono::milliseconds(10));
    }
};
asio::co_spawn(ioc, updateLoop(), asio::detached);

ioc.run();
server.stop(); // 等待线程池结束
```

---

#### Client（客户端）

```cpp
class Client : public KcpEndpoint
{
public:
    Client(IUdpTransport& transport, asio::any_io_executor exec);
  
    // 主动连接服务器
    std::shared_ptr<KcpSession> connect(uint32_t conv, const asio::ip::udp::endpoint& server_ep);
  
protected:
    uint32_t selectConv(const asio::ip::udp::endpoint&, std::span<const uint8_t> data) override;
};
```

**使用示例**:

```cpp
asio::io_context ioc;
AsioUdpTransport transport(ioc.get_executor(), 0); // 随机端口
Client client(transport, ioc.get_executor());

// 启动接收
asio::co_spawn(ioc, transport.recvLoop([&](const auto& from, auto data) {
    client.input(from, data);
}), asio::detached);

// 连接服务器
auto serverEp = asio::ip::udp::endpoint(asio::ip::make_address("127.0.0.1"), 9999);
auto session = client.connect(12345, serverEp); // Conv ID = 12345

// 业务协程
auto clientRoutine = [session]() -> asio::awaitable<void> {
    // 发送数据
    std::vector<uint8_t> msg = {0xAA, 0xBB};
    session->send(msg);
  
    // 接收响应
    auto result = co_await session->recv();
    if (result) {
        // 处理数据
    }
};
asio::co_spawn(ioc, clientRoutine(), asio::detached);

ioc.run();
```

---

### 5. 帧协议

#### FrameHeader

```cpp
#pragma pack(push, 1)
struct FrameHeader
{
    uint16_t magic = 0x55AA;  // 固定魔数
    uint16_t cmd = 0;         // 命令ID
    uint16_t length = 0;      // 载荷长度
};
#pragma pack(pop)
```

---

#### FrameCodec（编解码器）

```cpp
enum class CodecError {
    BufferTooSmall,
    InvalidMagic,
    IncompletePayload
};

// 编码
std::expected<std::span<uint8_t>, CodecError>
encodeFrame(std::span<uint8_t> buffer, uint16_t cmd, std::span<const uint8_t> payload);

// 解码
std::expected<std::tuple<uint16_t, std::span<const uint8_t>>, CodecError>
decodeFrame(std::span<const uint8_t> buffer);
```

**使用示例**:

```cpp
// 编码
std::vector<uint8_t> buffer(1024);
std::vector<uint8_t> payload = {1, 2, 3, 4};
auto result = encodeFrame(buffer, 0x1001, payload);

if (result) {
    session->send(*result); // 发送编码后的帧
}

// 解码
auto recvResult = co_await session->recv();
if (recvResult) {
    auto decoded = decodeFrame(*recvResult);
    if (decoded) {
        auto [cmd, payload] = *decoded;
        // cmd = 0x1001, payload = {1, 2, 3, 4}
    }
}
```

---

## 完整示例

### 服务端示例

```cpp
#include "src/net/App/Server.h"
#include "src/net/transport/AsioUdpTransport.h"
#include "src/net/protocol/FrameCodec.h"
#include <asio.hpp>

int main() {
    asio::io_context ioc;
  
    // 1. 创建传输层
    AsioUdpTransport transport(ioc.get_executor(), 9999);
  
    // 2. 创建服务器（4个工作线程）
    Server server(transport, ioc.get_executor(), 4);
  
    // 3. 启动接收循环
    asio::co_spawn(ioc, transport.recvLoop([&](const auto& from, auto data) {
        server.input(from, data);
    }), asio::detached);
  
    // 4. 定时更新会话
    asio::steady_timer timer(ioc, std::chrono::milliseconds(10));
    std::function<void(const asio::error_code&)> update;
    update = [&](const asio::error_code& ec) {
        if (!ec) {
            auto now = std::chrono::steady_clock::now().time_since_epoch();
            auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
            server.update(static_cast<uint32_t>(now_ms));
        
            timer.expires_after(std::chrono::milliseconds(10));
            timer.async_wait(update);
        }
    };
    timer.async_wait(update);
  
    // 5. 运行事件循环
    ioc.run();
  
    server.stop();
    return 0;
}
```

---

### 客户端示例

```cpp
#include "src/net/App/Client.h"
#include "src/net/transport/AsioUdpTransport.h"
#include "src/net/protocol/FrameCodec.h"

int main() {
    asio::io_context ioc;
  
    // 1. 创建传输层（端口0表示随机端口）
    AsioUdpTransport transport(ioc.get_executor(), 0);
  
    // 2. 创建客户端
    Client client(transport, ioc.get_executor());
  
    // 3. 启动接收
    asio::co_spawn(ioc, transport.recvLoop([&](const auto& from, auto data) {
        client.input(from, data);
    }), asio::detached);
  
    // 4. 连接服务器
    auto serverEp = asio::ip::udp::endpoint(asio::ip::make_address("127.0.0.1"), 9999);
    auto session = client.connect(10001, serverEp);
  
    // 5. 业务协程
    asio::co_spawn(ioc, [session]() -> asio::awaitable<void> {
        std::vector<uint8_t> sendBuffer(1024);
        std::vector<uint8_t> payload = {0x48, 0x65, 0x6C, 0x6C, 0x6F}; // "Hello"
    
        // 编码并发送
        auto encoded = encodeFrame(sendBuffer, 0x1001, payload);
        if (encoded) {
            session->send(*encoded);
        }
    
        // 等待响应
        auto result = co_await session->recv();
        if (result) {
            auto decoded = decodeFrame(*result);
            if (decoded) {
                auto [cmd, data] = *decoded;
                std::cout << "Received cmd: " << cmd << std::endl;
            }
        }
    }, asio::detached);
  
    // 6. 定时更新
    asio::steady_timer timer(ioc, std::chrono::milliseconds(10));
    std::function<void(const asio::error_code&)> update;
    update = [&](const asio::error_code& ec) {
        if (!ec) {
            auto now = std::chrono::steady_clock::now().time_since_epoch();
            auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
            client.update(static_cast<uint32_t>(now_ms));
        
            timer.expires_after(std::chrono::milliseconds(10));
            timer.async_wait(update);
        }
    };
    timer.async_wait(update);
  
    ioc.run();
    return 0;
}
```

---

## 最佳实践

### 1. 线程模型

```
IO 线程                           业务线程池
   |                                  |
   ├─ AsioUdpTransport.recv()         |
   ├─ KcpEndpoint.input()             |
   ├─ KcpSession.update() ────────────┼─> Server.playerRoutine()
   └─ 定时器                          |     (asio::strand 保证安全)
```

**建议**:

- IO 线程专注网络收发和 KCP 更新
- 业务逻辑放到线程池避免阻塞 IO
- 使用 `asio::strand` 保证单个会话的线程安全

---

### 2. 超时与清理

```cpp
// 默认30秒超时，可自定义
server.update(now_ms, std::chrono::seconds(60));
```

**建议**:

- 定期调用 `update()` 清理超时会话
- 在 `onSessionClosed()` 中释放业务资源

---

### 3. 错误处理

```cpp
auto result = co_await session->recv();
if (!result) {
    auto error = result.error();
    if (error == asio::error::operation_aborted) {
        // 会话被关闭
    }
}
```

**建议**:

- 使用 `std::expected` 检查返回值
- 捕获协程中的异常（已在 `Server::playerRoutine` 中实现）

---

### 4. 性能优化

**关键点**:

- `try_emplace` 避免重复查找
- `std::span` 避免不必要的拷贝
- `asio::experimental::channel` 实现无锁协程通信
- KCP 配置（可在 `KcpSession` 构造函数中调整）：

  ```cpp
  ikcp_nodelay(m_kcp, 1, 10, 2, 1); // 低延迟模式
  ikcp_wndsize(m_kcp, 128, 128);    // 窗口大小
  ```

---

## 测试

### 单元测试

项目提供完整的单元测试套件：

```bash
# 构建测试
cmake --build build --config Release --target net_tests

# 运行测试
./build/tests/unittest/Release/net_tests.exe
```

**测试覆盖**:

- `test_frame_codec.cpp`: 帧编解码测试
- `test_transport.cpp`: 传输层测试
- `test_kcp_session.cpp`: 会话层测试（8个测试用例）
- `test_kcp_server.cpp`: 服务端测试（7个测试用例）
- `test_kcp_client.cpp`: 客户端测试

---

## 常见问题

### Q: 如何自定义 Conv ID 分配策略？

**A**: 重写 `selectConv()` 方法：

```cpp
class MyServer : public Server {
protected:
    uint32_t selectConv(const asio::ip::udp::endpoint& from, 
                       std::span<const uint8_t> data) override {
        // 自定义逻辑，例如根据 endpoint 哈希
        return std::hash<std::string>{}(from.address().to_string() + ":" + std::to_string(from.port()));
    }
};
```

---

### Q: 如何实现心跳保活？

**A**: 在业务协程中定期发送心跳包：

```cpp
asio::awaitable<void> playerRoutine(std::shared_ptr<KcpSession> session) {
    asio::steady_timer heartbeat_timer(co_await asio::this_coro::executor);
  
    while (true) {
        heartbeat_timer.expires_after(std::chrono::seconds(5));
        co_await heartbeat_timer.async_wait(asio::use_awaitable);
    
        std::vector<uint8_t> heartbeat = {0xFF};
        session->send(heartbeat);
    }
}
```

---

### Q: 如何处理大包传输？

**A**: KCP 自动分片和重组，无需手动处理：

```cpp
std::vector<uint8_t> largeData(100000); // 100KB
session->send(largeData); // KCP 自动分片传输
```

---
