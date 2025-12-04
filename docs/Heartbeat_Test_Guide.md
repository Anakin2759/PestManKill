# 心跳包通信测试指南

## 概述

本测试演示了 PestManKill 的客户端-服务器心跳包通信机制，包括：

- UDP 网络连接
- 登录认证流程
- 自动心跳包发送（每 5 秒）
- 客户端超时检测（30 秒）
- 优雅断开连接

## 架构说明

### 服务器端 (`src/server/main.cpp`)

- 使用 `NetWorkManager` 处理 UDP 通信
- 使用 `ClientSessionManager` 管理客户端会话
- 监听端口：`8888`
- 支持的消息类型：
  - `LOGIN` - 客户端登录
  - `HEARTBEAT` - 心跳包
  - `LOGOUT` - 客户端登出

### 客户端端 (`src/client/main_test.cpp`)

- 使用 `GameClient` 高层 API
- 实现 `IServerMessageHandler` 接口处理服务器消息
- 自动心跳机制（登录成功后启动）
- 状态机：DISCONNECTED → CONNECTING → CONNECTED → AUTHENTICATED

## 构建说明

### 1. 配置 CMake

```powershell
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

### 2. 构建项目

```powershell
cmake --build build --config Release
```

构建完成后将生成：

- `build\src\server\Release\PestManKillServer.exe` - 服务器
- `build\src\client\Release\PestManKillClientTest.exe` - 测试客户端（无 GUI）

## 运行测试

### 步骤 1：启动服务器

在终端 1 中运行：

```powershell
.\build\src\server\Release\PestManKillServer.exe
```

**预期输出：**

```
========================================
服务器启动 - 心跳包通信测试
========================================
[info] 服务器启动，监听端口: 8888
[info] 服务器正在运行，监听端口: 8888
[info] 按 Ctrl+C 停止服务器
[info] 当前在线客户端数量: 0
```

### 步骤 2：启动客户端

在终端 2 中运行：

```powershell
.\build\src\client\Release\PestManKillClientTest.exe
```

**预期输出：**

```
========================================
客户端启动 - 心跳包通信测试
========================================
[info] 正在连接到服务器: 127.0.0.1:8888
[info] ✅ 已连接到服务器
[info] 发送登录请求: TestPlayer
[info] ✅ 登录成功! clientId=1, playerEntity=100
[info] 心跳启动，间隔: 5s
[info] 客户端正在运行...
[info] 心跳包将自动发送（登录成功后每 5 秒一次）
[debug] 收到心跳包响应
[info] 当前状态: AUTHENTICATED | ClientID: 1 | EntityID: 100
```

### 步骤 3：观察心跳通信

**服务器端日志：**

```
[info] 玩家登录: TestPlayer
[info] 发送登录响应: clientId=1, entityId=100
[debug] 收到心跳包 from 127.0.0.1:xxxxx
[debug] 收到心跳包 from 127.0.0.1:xxxxx
[info] 当前在线客户端数量: 1
```

**客户端日志：**

```
[debug] 收到消息: type=2, size=0
[debug] 收到消息: type=2, size=0
[info] 当前状态: AUTHENTICATED | ClientID: 1 | EntityID: 100
```

### 步骤 4：测试超时断线

1. 在客户端按 `Ctrl+C` 强制关闭（不发送 LOGOUT）
2. 等待 30 秒（超时时间）
3. 观察服务器日志：

```
[warn] 客户端超时: entity=100
[info] 当前在线客户端数量: 0
```

### 步骤 5：测试优雅断开

重新启动客户端，然后正常按 `Ctrl+C`：

**客户端日志：**

```
^C
收到停止信号，正在关闭客户端...
[info] 正在断开连接...
[info] 断开服务器连接
[info] 发送登出消息
[info] ❌ 已断开服务器连接
[info] 客户端已停止
```

**服务器日志：**

```
[info] 玩家登出: entity=100
```

## 通信协议详解

### 消息格式

每条消息由两部分组成：

1. **PacketHeader**（12 字节）

   ```
   ┌─────────┬─────────┬──────────┬──────────┐
   │ seq (4) │ ack (4) │ type (2) │ size (2) │
   └─────────┴─────────┴──────────┴──────────┘
   ```

2. **Payload**（可变长度）
   - 登录消息：纯文本玩家名称
   - 登录响应：JSON `{"clientId": 1, "entityId": 100, "message": "登录成功"}`
   - 心跳消息：空 payload

### 消息类型枚举

```cpp
enum class MessageType : uint16_t
{
    LOGIN = 1,          // 登录请求/响应
    LOGOUT = 2,         // 登出
    HEARTBEAT = 3,      // 心跳包
    // ... 其他游戏消息
};
```

### ACK 机制

- 可靠消息（LOGIN）使用 ACK 确认，超时重传（最多 3 次）
- 不可靠消息（HEARTBEAT）直接发送，不等待 ACK

## 关键参数

| 参数 | 值 | 说明 |
|------|-----|------|
| 服务器端口 | 8888 | UDP 监听端口 |
| 心跳间隔 | 5 秒 | 客户端发送心跳频率 |
| 超时时间 | 30 秒 | 服务器判定客户端掉线 |
| ACK 超时 | 1000 毫秒 | 可靠消息等待 ACK 时间 |
| 最大重试 | 3 次 | 可靠消息重传次数 |
| 线程池大小 | 4 线程 | ASIO 协程执行器 |

## 故障排查

### 问题 1：客户端无法连接

**症状：** 客户端日志显示 "登录请求发送失败"

**解决方案：**

1. 确认服务器已启动并监听 8888 端口
2. 检查防火墙是否阻止 UDP 8888
3. 确认 `127.0.0.1` 可达（本地回环）

### 问题 2：心跳包不发送

**症状：** 客户端状态停留在 `CONNECTED`，不转为 `AUTHENTICATED`

**解决方案：**

1. 检查登录是否成功（查找 "✅ 登录成功" 日志）
2. 心跳只在 `AUTHENTICATED` 或 `IN_GAME` 状态发送

### 问题 3：客户端被误判超时

**症状：** 客户端正常运行，但服务器报告超时

**解决方案：**

1. 检查网络延迟是否过高
2. 增加超时时间（修改 `ClientSessionManager.h` 中的 `HEARTBEAT_TIMEOUT`）
3. 减小心跳间隔

## 代码参考

### 服务器心跳处理

```cpp
case MessageType::HEARTBEAT:
{
    logger->debug("收到心跳包 from {}:{}", 
                 sender.address().to_string(), sender.port());

    // 回复心跳
    std::vector<uint8_t> emptyPayload;
    asio::co_spawn(
        threadPool.get_executor(),
        [networkManager, sender, emptyPayload]() -> asio::awaitable<void>
        {
            co_await networkManager->sendPacket(
                sender,
                static_cast<uint16_t>(MessageType::HEARTBEAT),
                emptyPayload
            );
        },
        asio::detached
    );
    break;
}
```

### 客户端自动心跳

```cpp
// 登录成功后启动心跳
void GameClient::handleLoginResponse(const uint8_t* data, size_t size)
{
    // ... 解析登录响应 ...
    
    m_state = ClientState::AUTHENTICATED;
    
    if (m_messageHandler)
    {
        m_messageHandler->onLoginSuccess(m_clientId, m_playerEntity);
    }

    // 启动心跳（默认 5 秒间隔）
    startHeartbeat();
}
```

## 扩展测试

### 多客户端测试

可以同时运行多个客户端实例：

```powershell
# 终端 2
.\build\src\client\Release\PestManKillClientTest.exe

# 终端 3
.\build\src\client\Release\PestManKillClientTest.exe

# 终端 4
.\build\src\client\Release\PestManKillClientTest.exe
```

服务器将显示：

```
[info] 当前在线客户端数量: 3
```

### 压力测试

修改 `main_test.cpp`，启动多个 `GameClient` 实例：

```cpp
std::vector<std::shared_ptr<GameClient>> clients;
for (int i = 0; i < 100; ++i)
{
    auto client = std::make_shared<GameClient>();
    client->connect("127.0.0.1", 8888);
    // ... 登录 ...
    clients.push_back(client);
}
```

## 总结

本测试验证了以下功能：

✅ UDP 客户端-服务器通信  
✅ 协程异步网络编程（C++23 `co_await`）  
✅ 可靠消息传输（ACK + 重传）  
✅ 自动心跳机制  
✅ 客户端会话管理  
✅ 超时检测和断线处理  
✅ 优雅关闭（RAII + 信号处理）  

这是完整游戏网络层的基础实现，后续可以在此基础上添加游戏逻辑消息。
