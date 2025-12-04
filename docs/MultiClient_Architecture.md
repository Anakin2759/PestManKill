# 服务器多客户端通信架构总结

## 概览

PestManKill 服务器使用三层架构来实现多客户端通信与区分：

```
┌─────────────────────────────────────────────────────────────┐
│                      客户端 A                                 │
│              (IP: 192.168.1.100:5000)                       │
└───────────────────────┬─────────────────────────────────────┘
                        │ UDP Packets
┌───────────────────────┼─────────────────────────────────────┐
│                       ↓                                      │
│  ┌─────────────────────────────────────────────────────┐   │
│  │        NetWorkManager (网络层)                        │   │
│  │  - 接收/发送 UDP 数据包                                │   │
│  │  - ACK 可靠传输机制                                    │   │
│  │  - 协程驱动的异步 I/O                                  │   │
│  └────────────────────┬────────────────────────────────┘   │
│                       │ endpoint 识别                       │
│  ┌────────────────────↓────────────────────────────────┐   │
│  │    ClientSessionManager (会话层)                      │   │
│  │  - endpoint ↔ entity 双向映射                        │   │
│  │  - 心跳检测与超时管理                                  │   │
│  │  - 客户端生命周期管理                                  │   │
│  └────────────────────┬────────────────────────────────┘   │
│                       │ entt::entity                       │
│  ┌────────────────────↓────────────────────────────────┐   │
│  │         GameContext (游戏逻辑层)                       │   │
│  │  - ECS registry & dispatcher                         │   │
│  │  - Components (Player, Attributes, etc.)             │   │
│  │  - Systems (GameFlowSystem, DamageSystem, etc.)      │   │
│  └──────────────────────────────────────────────────────┘   │
│                    服务器 (单实例)                           │
└──────────────────────────────────────────────────────────────┘
                        ↑
                        │ UDP Packets
┌───────────────────────┴─────────────────────────────────────┐
│              客户端 B (IP: 192.168.1.101:5001)               │
└──────────────────────────────────────────────────────────────┘
```

## 核心组件

### 1. NetWorkManager (网络层)

**职责**: UDP 数据包的收发与可靠传输

**文件**: `src/server/net/NetWorkManager.h`

**关键方法**:

```cpp
// 发送到指定 endpoint
asio::awaitable<void> sendPacket(endpoint, type, payload);
asio::awaitable<bool> sendReliablePacket(endpoint, type, payload, retries, timeout);

// 接收回调
void setPacketHandler(std::function<void(const uint8_t*, size_t, const endpoint&)>);
```

**特性**:

- 基于 ASIO + C++23 协程
- ACK 重传机制 (seq + endpoint 唯一标识)
- 线程池驱动的异步操作
- 事件驱动的 receiveLoop

### 2. ClientSessionManager (会话层)

**职责**: 客户端身份管理与映射

**文件**: `src/server/net/ClientSessionManager.h`

**核心数据结构**:

```cpp
struct ClientSession {
    asio::ip::udp::endpoint endpoint;       // 网络地址
    entt::entity playerEntity;              // 游戏实体
    std::chrono::time_point lastHeartbeat;  // 心跳时间
    uint32_t clientId;                      // 唯一 ID
    std::string playerName;                 // 玩家名
    bool isAuthenticated;                   // 认证状态
};
```

**关键映射**:

```cpp
absl::flat_hash_map<endpoint, clientId>     m_endpointToSession;
absl::flat_hash_map<entity, clientId>       m_entityToClientId;
absl::flat_hash_map<clientId, ClientSession> m_sessions;
```

**关键方法**:

```cpp
// 注册客户端
uint32_t registerClient(endpoint, entity, playerName);

// 双向查询
std::optional<entity> getPlayerEntity(endpoint);
std::optional<endpoint> getEndpoint(entity);

// 批量操作
std::vector<endpoint> getAllEndpoints();
std::vector<entity> checkTimeouts(); // 返回超时实体
```

### 3. GameContext (游戏逻辑层)

**职责**: ECS 核心，存储所有游戏状态

**文件**: `src/server/context/GameContext.h`

**组成**:

```cpp
struct GameContext {
    entt::registry registry;           // 实体组件存储
    entt::dispatcher dispatcher;       // 事件总线
    std::shared_ptr<spdlog::logger> logger;
    asio::thread_pool threadPool;      // 异步任务池
};
```

## 数据流示例

### 场景 1: 客户端登录

```
Client A                   NetWorkManager              ClientSessionManager        GameContext
   |                             |                             |                        |
   |--[LOGIN_REQUEST]----------->|                             |                        |
   |   (endpoint: 192.168.1.100) |                             |                        |
   |                             |--onPacketReceived()-------->|                        |
   |                             |   (data, size, endpoint)    |                        |
   |                             |                             |--createEntity()------->|
   |                             |                             |                   [entity=5]
   |                             |                             |<-----------------------|
   |                             |                             |                        |
   |                             |<--registerClient()----------|                        |
   |                             |   (endpoint, entity, name)  |                        |
   |                             |                        [clientId=1]                  |
   |                             |                             |                        |
   |<--[LOGIN_SUCCESS]-----------| sendReliablePacket()        |                        |
   |   (clientId, entityId)      |                             |                        |
```

**内部状态变化**:

```cpp
m_sessions[1] = {
    endpoint: 192.168.1.100:5000,
    playerEntity: 5,
    clientId: 1,
    playerName: "Player_A",
    isAuthenticated: true
};

m_endpointToSession[192.168.1.100:5000] = 1;
m_entityToClientId[5] = 1;
```

### 场景 2: 处理游戏行为

```
Client A                   NetWorkManager              ClientSessionManager        GameContext
   |                             |                             |                        |
   |--[USE_CARD]---------------->|                             |                        |
   |   (cardId=42)               |                             |                        |
   |                             |--onPacketReceived()-------->|                        |
   |                             |   (data, endpoint)          |                        |
   |                             |                  getPlayerEntity(endpoint)           |
   |                             |                        [return: entity=5]            |
   |                             |                             |--dispatcher.trigger()->|
   |                             |                             |   (UseCardEvent)       |
   |                             |                             |                   [Systems处理]
   |                             |<--getEndpoints()------------|                        |
   |                             |   (all except sender)       |                        |
   |<--[BROADCAST_EVENT]---------|                             |                        |
   |   (Player_A used card 42)   |                             |                        |
```

### 场景 3: 心跳超时处理

```
定时器 (10s)                ClientSessionManager        GameContext             NetWorkManager
   |                             |                        |                        |
   |--checkTimeouts()----------->|                        |                        |
   |                        [检查 lastHeartbeat]         |                        |
   |                        [发现 entity=5 超时]          |                        |
   |<--[timedOutPlayers]---------|                        |                        |
   |   {entity=5}                |                        |                        |
   |                             |--remove(endpoint)----->|                        |
   |                             |                        |                        |
   |--registry.destroy(5)------->|                        |--broadcast------------>|
   |                             |                 (Player_A disconnected)          |
```

## 消息路由流程

### 收到消息 → 找到玩家

```cpp
void onPacketReceived(const uint8_t* data, size_t size, const endpoint& sender) {
    // 1. 更新心跳
    m_sessionManager->updateHeartbeat(sender);
    
    // 2. 验证身份
    auto playerEntity = m_sessionManager->getPlayerEntity(sender);
    if (!playerEntity) {
        logger->warn("未认证客户端: {}", sender);
        return;
    }
    
    // 3. 读取玩家数据
    auto& playerInfo = registry.get<MetaPlayerInfo>(*playerEntity);
    auto& attributes = registry.get<Attributes>(*playerEntity);
    
    // 4. 处理业务逻辑
    handleUseCard(*playerEntity, data, size);
}
```

### 发送消息 → 指定玩家

```cpp
// 方式 1: 已知 endpoint
asio::awaitable<void> sendToEndpoint(endpoint target) {
    co_await networkManager->sendPacket(target, msgType, payload);
}

// 方式 2: 已知 entity
asio::awaitable<void> sendToPlayer(entt::entity player) {
    auto endpoint = sessionManager->getEndpoint(player);
    if (endpoint) {
        co_await networkManager->sendPacket(*endpoint, msgType, payload);
    }
}

// 方式 3: 广播到所有人
asio::awaitable<void> broadcastToAll() {
    auto endpoints = sessionManager->getAllEndpoints();
    for (auto& ep : endpoints) {
        co_await networkManager->sendPacket(ep, msgType, payload);
    }
}
```

## 线程安全保证

### 1. ClientSessionManager

- 所有方法使用 `std::scoped_lock` 保护
- 可从任意线程调用

### 2. NetWorkManager

- 内部 ACK 映射使用 `std::mutex` 保护
- 协程通过 `m_threadPool->get_executor()` 调度

### 3. GameContext

- `entt::registry` **非线程安全**
- 建议在单一线程（或同步访问）中操作
- 事件通过 `dispatcher` 解耦

## 性能考量

### 1. 查找复杂度

- `getPlayerEntity(endpoint)`: O(1) - flat_hash_map
- `getEndpoint(entity)`: O(1) - flat_hash_map
- `checkTimeouts()`: O(n) - n 为在线客户端数

### 2. 内存占用

- 每个客户端: ~200 bytes (ClientSession + 映射开销)
- 8 个客户端: ~1.6 KB

### 3. 并发模型

- 网络 I/O: 协程 + 线程池 (2 * CPU 核心数)
- 游戏逻辑: 事件驱动，单线程处理

## 最佳实践

### 1. 客户端注册时机

```cpp
// ❌ 错误: 收到任何消息就注册
void onPacketReceived(endpoint sender) {
    auto entity = sessionManager->getPlayerEntity(sender);
    if (!entity) {
        entity = registry.create();
        sessionManager->registerClient(sender, *entity, "Unknown");
    }
}

// ✅ 正确: 只在登录消息时注册
void handleLogin(endpoint sender, const LoginMessage& msg) {
    if (validateCredentials(msg)) {
        auto entity = createPlayerEntity(msg.playerName);
        sessionManager->registerClient(sender, entity, msg.playerName);
    }
}
```

### 2. 心跳更新时机

```cpp
// ✅ 在 packet handler 入口处统一更新
void onPacketReceived(const uint8_t* data, size_t size, const endpoint& sender) {
    sessionManager->updateHeartbeat(sender);  // 第一件事
    // ... 后续处理
}
```

### 3. 实体销毁顺序

```cpp
// ✅ 正确顺序
void kickPlayer(entt::entity player) {
    auto endpoint = sessionManager->getEndpoint(player);
    
    // 1. 发送通知
    sendKickMessage(*endpoint, reason);
    
    // 2. 移除会话
    sessionManager->disconnectClient(*endpoint);
    
    // 3. 销毁实体
    registry.destroy(player);
}
```

### 4. 广播时排除发送者

```cpp
void broadcastExcludeSender(entt::entity sender, MessageType type, Payload payload) {
    auto senderEndpoint = sessionManager->getEndpoint(sender);
    auto allEndpoints = sessionManager->getAllEndpoints();
    
    for (const auto& ep : allEndpoints) {
        if (senderEndpoint && ep == *senderEndpoint) continue;  // 跳过
        networkManager->sendPacket(ep, type, payload);
    }
}
```

## 完整示例代码

参考文件:

- **基础架构**: `src/server/net/NetWorkManager.h`
- **会话管理**: `src/server/net/ClientSessionManager.h`
- **使用指南**: `src/server/net/ClientSessionManager_Usage.md`
- **完整示例**: `src/server/GameServer_Example.h`

## 扩展方向

### 1. 房间系统

```cpp
class RoomManager {
    struct Room {
        uint32_t roomId;
        std::vector<entt::entity> players;
    };
    
    absl::flat_hash_map<uint32_t, Room> m_rooms;
    
    void broadcastToRoom(uint32_t roomId, MessageType type, Payload payload);
};
```

### 2. 权限分级

```cpp
struct ClientSession {
    // ... 现有字段
    enum class Role { Guest, Player, Admin } role;
};

bool hasPermission(endpoint sender, Permission perm) {
    auto session = sessionManager->getSession(sender);
    return session && session->role >= perm.requiredRole;
}
```

### 3. 消息队列

```cpp
class MessageQueue {
    void enqueue(entt::entity target, MessageType type, Payload payload);
    void flushAll();  // 批量发送，减少系统调用
};
```

## 调试技巧

### 1. 查看在线客户端

```cpp
void printOnlineClients() {
    auto endpoints = sessionManager->getAllEndpoints();
    logger->info("在线客户端: {}", endpoints.size());
    
    for (const auto& ep : endpoints) {
        auto session = sessionManager->getSession(ep);
        if (session) {
            logger->info("  - {} (Entity={}, ClientID={})", 
                        session->playerName, 
                        static_cast<uint32_t>(session->playerEntity),
                        session->clientId);
        }
    }
}
```

### 2. 模拟网络延迟

```cpp
asio::awaitable<void> sendWithDelay(endpoint target, MessageType type, Payload payload) {
    auto timer = std::make_shared<asio::steady_timer>(executor);
    timer->expires_after(std::chrono::milliseconds(100));  // 模拟 100ms 延迟
    co_await timer->async_wait(asio::use_awaitable);
    
    co_await networkManager->sendPacket(target, type, payload);
}
```

### 3. 日志级别控制

```cpp
// 调试模式: 记录所有包
logger->set_level(spdlog::level::debug);

// 生产模式: 只记录重要事件
logger->set_level(spdlog::level::info);
```

## 总结

通过 **NetWorkManager + ClientSessionManager + GameContext** 三层架构，实现了:

✅ **客户端唯一标识**: endpoint (IP+端口)  
✅ **游戏实体映射**: endpoint ↔ entity 双向查询  
✅ **生命周期管理**: 注册、心跳、超时、断开  
✅ **消息路由**: 单播、广播、组播  
✅ **线程安全**: 所有管理器内部同步  
✅ **可扩展性**: 支持房间、权限、队列等扩展  

这套架构清晰分离了**网络传输**、**会话管理**和**游戏逻辑**三个关注点，便于维护和扩展。
