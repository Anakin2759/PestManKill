# ClientSessionManager 使用指南

## 概述

`ClientSessionManager` 是服务器端客户端会话管理器，负责：

- 将 UDP `endpoint` 映射到游戏 `entt::entity`
- 管理客户端生命周期（注册、心跳、超时）
- 提供双向查询（endpoint ↔ entity）
- 支持批量操作（广播、批量查询）

## 核心概念

### 三层映射关系

```
UDP endpoint <---> ClientSession <---> entt::entity (Player)
     ^                   |                    ^
     |                   |                    |
  网络层            会话管理层             游戏逻辑层
```

- **endpoint**: ASIO 的 UDP 地址（IP + 端口）
- **ClientSession**: 会话信息（ID、心跳、认证状态）
- **entity**: EnTT 的玩家实体（关联 Components）

## 基础用法

### 1. 创建管理器

```cpp
#include "src/server/net/ClientSessionManager.h"
#include "src/server/context/GameContext.h"

class GameServer
{
    GameContext m_context;
    std::shared_ptr<ClientSessionManager> m_sessionManager;
    
public:
    GameServer() 
        : m_sessionManager(std::make_shared<ClientSessionManager>(m_context.logger))
    {
    }
};
```

### 2. 注册客户端

当收到客户端的**登录请求**时：

```cpp
void onClientLogin(const uint8_t* data, size_t size, const asio::ip::udp::endpoint& sender)
{
    // 解析登录消息
    std::string playerName = parsePlayerName(data, size);
    
    // 创建玩家实体
    auto playerEntity = m_context.registry.create();
    m_context.registry.emplace<MetaPlayerInfo>(playerEntity, playerName, DEFAULT_PLAYER_ID);
    m_context.registry.emplace<Attributes>(playerEntity, 4, 4, 1);
    
    // 注册会话
    uint32_t clientId = m_sessionManager->registerClient(sender, playerEntity, playerName);
    
    m_context.logger->info("玩家 {} 登录成功，ClientID: {}", playerName, clientId);
}
```

### 3. 处理客户端消息

在 `NetWorkManager` 的 packet handler 中：

```cpp
void setupNetworking()
{
    m_networkManager->setPacketHandler(
        [this](const uint8_t* data, size_t size, const asio::ip::udp::endpoint& sender)
        {
            // 更新心跳
            m_sessionManager->updateHeartbeat(sender);
            
            // 获取玩家实体
            auto playerEntity = m_sessionManager->getPlayerEntity(sender);
            if (!playerEntity)
            {
                m_context.logger->warn("收到未认证客户端的消息: {}", sender.address().to_string());
                return;
            }
            
            // 处理游戏逻辑
            handleGameMessage(*playerEntity, data, size);
        }
    );
}
```

### 4. 发送消息到指定玩家

```cpp
// 方式 1: 通过玩家实体发送
asio::awaitable<void> sendToPlayer(entt::entity playerEntity, uint16_t msgType, std::vector<uint8_t> payload)
{
    auto endpoint = m_sessionManager->getEndpoint(playerEntity);
    if (!endpoint)
    {
        m_context.logger->error("玩家实体 {} 未找到对应的 endpoint", static_cast<uint32_t>(playerEntity));
        co_return;
    }
    
    co_await m_networkManager->sendReliablePacket(*endpoint, msgType, payload);
}

// 方式 2: 直接通过 endpoint 发送（已知 endpoint 的情况）
asio::awaitable<void> replyToClient(const asio::ip::udp::endpoint& sender, 
                                    uint16_t msgType, 
                                    std::vector<uint8_t> payload)
{
    co_await m_networkManager->sendPacket(sender, msgType, payload);
}
```

### 5. 广播消息到所有玩家

```cpp
asio::awaitable<void> broadcastGameStart()
{
    auto endpoints = m_sessionManager->getAllEndpoints();
    
    std::vector<uint8_t> payload = buildGameStartMessage();
    uint16_t msgType = static_cast<uint16_t>(MessageType::GAME_START);
    
    // 并发发送到所有客户端
    std::vector<asio::awaitable<void>> tasks;
    for (const auto& endpoint : endpoints)
    {
        tasks.push_back(m_networkManager->sendPacket(endpoint, msgType, payload));
    }
    
    // 等待所有发送完成（可选）
    // co_await asio::experimental::wait_for_all(std::move(tasks));
    
    m_context.logger->info("广播游戏开始消息到 {} 个客户端", endpoints.size());
}
```

### 6. 发送到特定玩家组

```cpp
asio::awaitable<void> notifyTeam(const std::vector<entt::entity>& teamPlayers, 
                                 uint16_t msgType, 
                                 std::vector<uint8_t> payload)
{
    auto endpoints = m_sessionManager->getEndpoints(teamPlayers);
    
    for (const auto& endpoint : endpoints)
    {
        co_await m_networkManager->sendPacket(endpoint, msgType, payload);
    }
    
    m_context.logger->info("通知小队 {} 个玩家", endpoints.size());
}
```

### 7. 心跳检测与超时清理

在主循环或定时器中：

```cpp
void gameLoop()
{
    // 每 10 秒检查一次超时
    static auto lastCheck = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    
    if (now - lastCheck > std::chrono::seconds(10))
    {
        auto timedOutPlayers = m_sessionManager->checkTimeouts();
        
        // 处理超时玩家
        for (auto playerEntity : timedOutPlayers)
        {
            m_context.logger->info("玩家 {} 超时断线", static_cast<uint32_t>(playerEntity));
            
            // 触发断线事件
            m_context.dispatcher.trigger<events::PlayerDisconnected>(playerEntity);
            
            // 销毁实体
            m_context.registry.destroy(playerEntity);
        }
        
        lastCheck = now;
    }
}
```

### 8. 主动断开客户端

```cpp
void kickPlayer(entt::entity playerEntity, const std::string& reason)
{
    auto endpoint = m_sessionManager->getEndpoint(playerEntity);
    if (!endpoint)
    {
        return;
    }
    
    // 发送踢出消息
    std::vector<uint8_t> payload = buildKickMessage(reason);
    asio::co_spawn(
        m_context.threadPool.get_executor(),
        [this, endpoint = *endpoint, payload]() -> asio::awaitable<void>
        {
            co_await m_networkManager->sendReliablePacket(
                endpoint, 
                static_cast<uint16_t>(MessageType::KICKED), 
                payload
            );
        },
        asio::detached
    );
    
    // 断开会话
    m_sessionManager->disconnectClient(*endpoint);
    
    // 销毁实体
    m_context.registry.destroy(playerEntity);
    
    m_context.logger->info("踢出玩家 {}, 原因: {}", static_cast<uint32_t>(playerEntity), reason);
}
```

## 完整示例：服务器主类

```cpp
#include "src/server/net/NetWorkManager.h"
#include "src/server/net/ClientSessionManager.h"
#include "src/server/context/GameContext.h"
#include "src/shared/messages/UseCardMessage.h"

class GameServer : public std::enable_shared_from_this<GameServer>
{
public:
    GameServer(uint16_t port) : m_port(port)
    {
        m_networkManager = std::make_shared<NetWorkManager>(
            m_context.threadPool, 
            m_context.logger
        );
        
        m_sessionManager = std::make_shared<ClientSessionManager>(
            m_context.logger
        );
    }
    
    void start()
    {
        // 设置消息处理器
        m_networkManager->setPacketHandler(
            [self = shared_from_this()](const uint8_t* data, size_t size, const asio::ip::udp::endpoint& sender)
            {
                self->onPacketReceived(data, size, sender);
            }
        );
        
        // 启动网络
        m_networkManager->start(m_port);
        m_context.logger->info("游戏服务器启动在端口 {}", m_port);
        
        // 启动心跳检测
        startHeartbeatTimer();
    }
    
private:
    void onPacketReceived(const uint8_t* data, size_t size, const asio::ip::udp::endpoint& sender)
    {
        if (size < sizeof(uint16_t)) return;
        
        // 解析消息类型
        uint16_t msgType;
        std::memcpy(&msgType, data, sizeof(uint16_t));
        
        auto messageType = static_cast<MessageType>(msgType);
        
        switch (messageType)
        {
            case MessageType::LOGIN:
                handleLogin(data + sizeof(uint16_t), size - sizeof(uint16_t), sender);
                break;
                
            case MessageType::HEARTBEAT:
                m_sessionManager->updateHeartbeat(sender);
                break;
                
            case MessageType::USE_CARD:
            {
                auto playerEntity = m_sessionManager->getPlayerEntity(sender);
                if (playerEntity)
                {
                    handleUseCard(*playerEntity, data + sizeof(uint16_t), size - sizeof(uint16_t));
                }
                break;
            }
            
            default:
                m_context.logger->warn("未知消息类型: {}", msgType);
        }
    }
    
    void handleLogin(const uint8_t* data, size_t size, const asio::ip::udp::endpoint& sender)
    {
        // 解析玩家名称
        std::string playerName(reinterpret_cast<const char*>(data), size);
        
        // 创建玩家实体
        auto playerEntity = m_context.registry.create();
        m_context.registry.emplace<MetaPlayerInfo>(playerEntity, playerName);
        
        // 注册会话
        uint32_t clientId = m_sessionManager->registerClient(sender, playerEntity, playerName);
        
        // 发送登录成功响应
        asio::co_spawn(
            m_context.threadPool.get_executor(),
            [self = shared_from_this(), sender, clientId]() -> asio::awaitable<void>
            {
                std::vector<uint8_t> response;
                response.resize(sizeof(uint32_t));
                std::memcpy(response.data(), &clientId, sizeof(uint32_t));
                
                co_await self->m_networkManager->sendReliablePacket(
                    sender,
                    static_cast<uint16_t>(MessageType::LOGIN_SUCCESS),
                    response
                );
            },
            asio::detached
        );
    }
    
    void handleUseCard(entt::entity playerEntity, const uint8_t* data, size_t size)
    {
        // 处理出牌逻辑
        UseCardMessage msg;
        // 反序列化消息...
        
        m_context.logger->info("玩家 {} 使用卡牌", static_cast<uint32_t>(playerEntity));
        
        // 触发游戏事件
        m_context.dispatcher.trigger<events::UseCardEvent>(playerEntity, msg.cardEntity);
    }
    
    void startHeartbeatTimer()
    {
        // 使用 ASIO timer 定期检查超时
        auto timer = std::make_shared<asio::steady_timer>(m_context.threadPool.get_executor());
        scheduleHeartbeatCheck(timer);
    }
    
    void scheduleHeartbeatCheck(std::shared_ptr<asio::steady_timer> timer)
    {
        timer->expires_after(std::chrono::seconds(10));
        timer->async_wait(
            [self = shared_from_this(), timer](const asio::error_code& error)
            {
                if (!error)
                {
                    auto timedOutPlayers = self->m_sessionManager->checkTimeouts();
                    for (auto player : timedOutPlayers)
                    {
                        self->m_context.registry.destroy(player);
                    }
                    
                    // 递归调度下一次检查
                    self->scheduleHeartbeatCheck(timer);
                }
            }
        );
    }
    
    GameContext m_context;
    std::shared_ptr<NetWorkManager> m_networkManager;
    std::shared_ptr<ClientSessionManager> m_sessionManager;
    uint16_t m_port;
};
```

## 高级用法

### 1. 房间/频道隔离

```cpp
class RoomManager
{
    struct Room
    {
        uint32_t roomId;
        std::vector<entt::entity> players;
    };
    
    absl::flat_hash_map<uint32_t, Room> m_rooms;
    
    asio::awaitable<void> broadcastToRoom(uint32_t roomId, uint16_t msgType, std::vector<uint8_t> payload)
    {
        if (auto iter = m_rooms.find(roomId); iter != m_rooms.end())
        {
            auto endpoints = m_sessionManager->getEndpoints(iter->second.players);
            
            for (const auto& endpoint : endpoints)
            {
                co_await m_networkManager->sendPacket(endpoint, msgType, payload);
            }
        }
    }
};
```

### 2. 消息队列与批处理

```cpp
class MessageBatcher
{
    struct BatchedMessage
    {
        uint16_t msgType;
        std::vector<uint8_t> payload;
        std::vector<asio::ip::udp::endpoint> targets;
    };
    
    std::vector<BatchedMessage> m_pendingMessages;
    
    void queueMessage(entt::entity target, uint16_t msgType, std::vector<uint8_t> payload)
    {
        auto endpoint = m_sessionManager->getEndpoint(target);
        if (endpoint)
        {
            m_pendingMessages.push_back({msgType, payload, {*endpoint}});
        }
    }
    
    asio::awaitable<void> flushBatch()
    {
        for (auto& msg : m_pendingMessages)
        {
            for (const auto& endpoint : msg.targets)
            {
                co_await m_networkManager->sendPacket(endpoint, msg.msgType, msg.payload);
            }
        }
        m_pendingMessages.clear();
    }
};
```

## 注意事项

1. **线程安全**: `ClientSessionManager` 内部使用 `std::mutex`，所有方法都是线程安全的
2. **心跳超时**: 默认 30 秒，可通过 `HEARTBEAT_TIMEOUT` 调整
3. **实体生命周期**: 超时后需要手动销毁 `entt::entity`
4. **endpoint 唯一性**: UDP 连接由 IP+端口唯一标识，客户端重启后会被视为新连接
5. **内存管理**: 断开连接时记得清理对应的 Components

## 最佳实践

1. **登录流程**: 先认证，再注册到 SessionManager
2. **定期心跳**: 客户端每 5-10 秒发送心跳包
3. **优雅断开**: 发送 LOGOUT 消息后再调用 `disconnectClient()`
4. **错误处理**: endpoint 查询失败时检查客户端是否已断开
5. **日志记录**: 关键操作（登录、断开、超时）记录日志
