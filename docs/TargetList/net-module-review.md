# Net 模块评估与优化优先级

## 覆盖范围

- 传输层：AsioUdpTransport
- 会话层：KcpSession
- 端点管理：KcpEndpoint、Server、Client

相关文件：

- [src/net/transport/AsioUdpTransport.h](src/net/transport/AsioUdpTransport.h)
- [src/net/transport/AsioUdpTransport.cpp](src/net/transport/AsioUdpTransport.cpp)
- [src/net/Session/KcpSession.h](src/net/Session/KcpSession.h)
- [src/net/Session/KcpSession.cpp](src/net/Session/KcpSession.cpp)
- [src/net/App/KcpEndpoint.h](src/net/App/KcpEndpoint.h)
- [src/net/App/Server.cpp](src/net/App/Server.cpp)
- [src/net/App/Client.cpp](src/net/App/Client.cpp)

## 现状简评（全局视角）

- 架构分层清晰：UDP 传输 / KCP 会话 / Endpoint 管理 / 业务协程解耦。整体结构适合扩展成可靠 UDP 通信层。
- 协程通路已建立：KCP 到业务层的 `channel` + `co_spawn` 路径完整，便于组织业务逻辑。
- 仍存在可靠性与可观测性短板：异常处理、会话生命周期、背压与日志体系未完善。

## 优化优先级

### P0（必须优先处理）

1. **会话生命周期与协程退出的确定性**
   - `KcpSession::recv()` 当前只在 channel 抛错时退出，`KcpEndpoint` 清理 session 时未主动通知协程退出，易出现协程悬挂或资源泄露。
   - 方向：在会话关闭时关闭 `channel` 或设置错误码使 `recv()` 退出。

2. **背压/丢包策略明确化**
   - `input()` 中使用 `try_send`，通道满时静默丢包，缺少统计或策略。
   - 方向：记录丢包指标或改为等待式发送，或提供可配置的丢包策略。

3. **日志与错误路径标准化**
   - `Server` 与 `KcpEndpoint` 仍使用 `std::cout`；项目约定要求使用 `m_context->logger`。
   - 方向：网络层日志统一走项目日志系统，确保可观测性与等级控制。

### P1（高价值优化）

1. **KCP 更新调度机制完善**
   - `update()` 依赖外部调用，未提供基于 `check()` 的调度示例，易出现过度更新或遗漏更新。
   - 方向：提供定时器驱动示例或内部驱动策略。

2. **传输层发送/接收错误处理**
   - UDP `send` 同步执行，缺少错误码反馈与统计。
   - 方向：提供异步发送或返回错误码；统一异常/错误路径。

3. **会话超时与活跃度判断**
   - `KcpEndpoint::update()` 使用最后一次 UDP 输入时间作为活跃度，未考虑 KCP 内部状态。
   - 方向：结合 KCP `check()` 或业务心跳，提高超时判定的准确性。

### P2（可延后）

1. **内存与缓冲复用**
   - `input()` 每次创建 `recvBuffer`，可能造成频繁分配。
   - 方向：引入缓冲池或复用策略。

2. **API 体验完善**
   - `KcpSession` 缺少 `getConv()`/`peer()` 等便捷接口。
   - 方向：完善只读访问接口，便于调试与业务层使用。

## 使用示例（最小可运行结构）

> 目标：展示 UDP 接收 → KCP endpoint → 业务协程的最小链路。

### 服务器侧示例

```cpp
#include <asio.hpp>
#include "src/net/transport/AsioUdpTransport.h"
#include "src/net/App/Server.h"

asio::awaitable<void> udpRecvLoop(AsioUdpTransport& transport, Server& server)
{
    co_await transport.recvLoop([&](const asio::ip::udp::endpoint& from, std::span<const uint8_t> data)
    {
        server.input(from, data);
    });
}

int main()
{
    asio::io_context ioc;
    AsioUdpTransport transport(ioc.get_executor(), 20000);
    Server server(transport, ioc.get_executor(), 4);

    // 启动 UDP 接收协程
    asio::co_spawn(ioc, udpRecvLoop(transport, server), asio::detached);

    // 业务侧定时更新 KCP（简单示例）
    asio::steady_timer timer(ioc);
    asio::co_spawn(ioc,
                   [&]() -> asio::awaitable<void>
                   {
                       for (;;)
                       {
                           timer.expires_after(std::chrono::milliseconds(10));
                           co_await timer.async_wait(asio::use_awaitable);
                           const auto now_ms = static_cast<uint32_t>(
                               std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now().time_since_epoch())
                                   .count());
                           server.update(now_ms);
                       }
                   }(),
                   asio::detached);

    ioc.run();
    return 0;
}
```

### 客户端侧示例

```cpp
#include <asio.hpp>
#include "src/net/transport/AsioUdpTransport.h"
#include "src/net/App/Client.h"

int main()
{
    asio::io_context ioc;
    AsioUdpTransport transport(ioc.get_executor(), 0); // 0 表示系统分配端口
    Client client(transport, ioc.get_executor());

    asio::ip::udp::endpoint server_ep(asio::ip::make_address("127.0.0.1"), 20000);
    auto session = client.connect(1001, server_ep);

    // 发送示例
    std::vector<uint8_t> payload = {1, 2, 3, 4};
    session->send(payload);

    // 接收协程示例
    asio::co_spawn(ioc,
                   [session]() -> asio::awaitable<void>
                   {
                       for (;;)
                       {
                           auto result = co_await session->recv();
                           if (!result)
                           {
                               co_return;
                           }
                           // 处理数据
                       }
                   }(),
                   asio::detached);

    // 驱动接收与更新
    asio::co_spawn(ioc,
                   [&]() -> asio::awaitable<void>
                   {
                       co_await transport.recvLoop([&](const asio::ip::udp::endpoint& from, std::span<const uint8_t> data)
                       {
                           client.input(from, data);
                       });
                   }(),
                   asio::detached);

    asio::steady_timer timer(ioc);
    asio::co_spawn(ioc,
                   [&]() -> asio::awaitable<void>
                   {
                       for (;;)
                       {
                           timer.expires_after(std::chrono::milliseconds(10));
                           co_await timer.async_wait(asio::use_awaitable);
                           const auto now_ms = static_cast<uint32_t>(
                               std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now().time_since_epoch())
                                   .count());
                           client.update(now_ms);
                       }
                   }(),
                   asio::detached);

    ioc.run();
    return 0;
}
```

## 注意事项

- `selectConv()` 默认使用 `peekConv` 从包头读取 `conv`，确保发送端包结构与 KCP 一致。
- `KcpSession::recv()` 返回 `std::expected`，务必处理错误路径。
- 业务协程与 KCP 驱动协程建议运行在不同执行器以减少阻塞。
