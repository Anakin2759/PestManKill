# 客户端掉线标记与重连机制

## 设计理念

在游戏进行中，客户端断线时**不立即移除会话和实体**，而是：

1. 标记为"掉线"状态 (`isDisconnected = true`)
2. 保留玩家实体和游戏状态
3. 允许客户端重新连接恢复游戏
4. 游戏结束时统一清理所有会话

## 核心 API

### 1. 掉线检测（标记，不移除）

```cpp
// 定期调用（如每 10 秒）
std::vector<entt::entity> newlyDisconnected = sessionManager->checkTimeouts();

// 返回新掉线的玩家实体列表
for (auto playerEntity : newlyDisconnected) {
    // 通知其他玩家某人掉线
    broadcastPlayerDisconnected(playerEntity);
    
    // 触发掉线事件（可选）
    dispatcher.trigger<events::PlayerDisconnected>(playerEntity);
    
    // 注意：不销毁实体！
}
```

### 2. 查询掉线状态

```cpp
// 通过 endpoint 查询
bool isDisconnected = sessionManager->isClientDisconnected(endpoint);

// 通过玩家实体查询
bool isDisconnected = sessionManager->isPlayerDisconnected(playerEntity);
```

### 3. 重新连接

```cpp
// 客户端重新发送登录请求时
if (sessionManager->reconnectClient(endpoint)) {
    logger->info("玩家重连成功");
    
    // 发送游戏状态同步
    sendGameStateSyncToClient(endpoint);
    
    // 通知其他玩家重连
    broadcastPlayerReconnected(playerEntity);
} else {
    // 新玩家登录
    registerNewClient(endpoint, playerName);
}
```

### 4. 获取在线玩家（排除掉线）

```cpp
// 默认行为：只返回在线玩家
auto onlineEndpoints = sessionManager->getAllEndpoints(); // excludeDisconnected = true

// 获取所有玩家（包括掉线的）
auto allEndpoints = sessionManager->getAllEndpoints(false);

// 获取所有掉线的玩家
auto disconnectedEndpoints = sessionManager->getDisconnectedEndpoints();
```

### 5. 游戏结束清理

```cpp
void onGameEnd() {
    // 获取所有玩家（包括掉线的）
    auto allEndpoints = sessionManager->getAllEndpoints(false);
    
    std::vector<entt::entity> allPlayers;
    for (const auto& endpoint : allEndpoints) {
        auto entity = sessionManager->getPlayerEntity(endpoint, true); // 允许掉线的
        if (entity) {
            allPlayers.push_back(*entity);
        }
    }
    
    // 清理所有会话
    sessionManager->clearAllSessions();
    
    // 销毁所有实体
    for (auto entity : allPlayers) {
        registry.destroy(entity);
    }
}
```

## 完整流程示例

### 场景 1：玩家掉线

```
时刻 T0: 玩家 A 正常游戏
    - session.isDisconnected = false
    - session.lastHeartbeat = T0

时刻 T30: 心跳超时（30秒未收到消息）
    - checkTimeouts() 检测到超时
    - session.isDisconnected = true
    - session.disconnectedTime = T30
    - 返回 playerEntity 到 newlyDisconnected 列表
    
服务器处理:
    - 广播: "玩家 A 掉线了"
    - 保留 playerEntity 和所有 Components
    - 游戏继续进行（可能跳过该玩家回合）

时刻 T45: 玩家 A 重新连接
    - 收到 LOGIN_REQUEST
    - reconnectClient(endpoint) 返回 true
    - session.isDisconnected = false
    - session.lastHeartbeat = T45
    
服务器处理:
    - 广播: "玩家 A 重新连接"
    - 发送游戏状态同步给玩家 A
    - 玩家 A 恢复游戏
```

### 场景 2：游戏正常结束

```
游戏进行中:
    - 玩家 A, B, C 在线
    - 玩家 D 掉线 (isDisconnected = true)
    - 玩家 E 掉线 (isDisconnected = true)

游戏结束:
    1. 触发 onGameEnd()
    2. getAllEndpoints(false) → [A, B, C, D, E]
    3. 收集所有玩家实体: [entity_A, entity_B, ..., entity_E]
    4. clearAllSessions() → 清空所有映射
    5. 销毁所有实体
    6. 服务器回到等待新游戏状态
```

## 使用示例代码

### 服务器主循环

```cpp
class GameServer {
    void gameLoop() {
        while (m_gameRunning) {
            // 每 10 秒检查一次超时
            static auto lastCheck = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            
            if (now - lastCheck > std::chrono::seconds(10)) {
                checkClientTimeouts();
                lastCheck = now;
            }
            
            // 游戏逻辑更新...
            updateGameLogic();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        
        // 游戏结束
        onGameEnd();
    }
    
    void checkClientTimeouts() {
        auto newlyDisconnected = m_sessionManager->checkTimeouts();
        
        for (auto playerEntity : newlyDisconnected) {
            auto* playerInfo = m_context.registry.try_get<MetaPlayerInfo>(playerEntity);
            std::string playerName = playerInfo ? playerInfo->playerName : "Unknown";
            
            m_context.logger->warn("玩家 {} 掉线", playerName);
            
            // 通知其他在线玩家
            broadcastPlayerDisconnected(playerName);
            
            // 标记实体为掉线状态（可选）
            m_context.registry.emplace_or_replace<DisconnectedTag>(playerEntity);
        }
    }
};
```

### 处理登录请求（支持重连）

```cpp
void handleLoginRequest(const uint8_t* data, size_t size, const asio::ip::udp::endpoint& sender) {
    std::string playerName(reinterpret_cast<const char*>(data), size);
    
    // 尝试重连
    if (m_sessionManager->reconnectClient(sender)) {
        auto playerEntity = m_sessionManager->getPlayerEntity(sender);
        if (playerEntity) {
            m_context.logger->info("玩家 {} 重连成功", playerName);
            
            // 移除掉线标记（如果有）
            m_context.registry.remove<DisconnectedTag>(*playerEntity);
            
            // 发送游戏状态同步
            sendGameStateSync(sender, *playerEntity);
            
            // 通知其他玩家
            broadcastPlayerReconnected(playerName);
            
            return;
        }
    }
    
    // 新玩家登录
    if (m_sessionManager->getClientCount() >= MAX_CLIENTS) {
        sendLoginFailed(sender, "服务器已满");
        return;
    }
    
    auto playerEntity = createPlayerEntity(playerName);
    uint32_t clientId = m_sessionManager->registerClient(sender, playerEntity, playerName);
    
    sendLoginSuccess(sender, clientId, playerEntity);
    broadcastPlayerJoined(playerName);
}
```

### 广播消息（排除掉线玩家）

```cpp
void broadcastGameEvent(const std::string& eventMessage) {
    std::vector<uint8_t> payload(eventMessage.begin(), eventMessage.end());
    
    // 只发送给在线玩家
    auto onlineEndpoints = m_sessionManager->getAllEndpoints(); // 默认排除掉线的
    
    asio::co_spawn(
        m_context.threadPool.get_executor(),
        [this, onlineEndpoints, payload]() -> asio::awaitable<void> {
            for (const auto& endpoint : onlineEndpoints) {
                co_await m_networkManager->sendPacket(
                    endpoint, 
                    MSG_TYPE_EVENT, 
                    payload
                );
            }
        },
        asio::detached
    );
}
```

### 游戏逻辑处理（跳过掉线玩家）

```cpp
void processTurn(entt::entity currentPlayer) {
    // 检查当前玩家是否掉线
    if (m_sessionManager->isPlayerDisconnected(currentPlayer)) {
        m_context.logger->info("跳过掉线玩家 {}", static_cast<uint32_t>(currentPlayer));
        
        // 自动跳过回合或做 AI 托管
        skipTurn(currentPlayer);
        
        // 切换到下一个在线玩家
        auto nextPlayer = getNextOnlinePlayer(currentPlayer);
        processTurn(nextPlayer);
        return;
    }
    
    // 正常处理回合
    handlePlayerTurn(currentPlayer);
}

entt::entity getNextOnlinePlayer(entt::entity currentPlayer) {
    auto allPlayers = m_gameData->playerQueue;
    
    // 从当前玩家的下一个开始查找
    bool foundCurrent = false;
    for (auto player : allPlayers) {
        if (foundCurrent) {
            // 找到在线玩家
            if (!m_sessionManager->isPlayerDisconnected(player)) {
                return player;
            }
        }
        if (player == currentPlayer) {
            foundCurrent = true;
        }
    }
    
    // 循环到开头查找
    for (auto player : allPlayers) {
        if (!m_sessionManager->isPlayerDisconnected(player)) {
            return player;
        }
    }
    
    return entt::null; // 所有玩家都掉线
}
```

### 游戏结束清理

```cpp
void onGameEnd() {
    m_context.logger->info("游戏结束，清理所有客户端");
    
    // 收集所有玩家实体（包括掉线的）
    std::vector<entt::entity> allPlayers;
    auto allEndpoints = m_sessionManager->getAllEndpoints(false); // 包括掉线的
    
    for (const auto& endpoint : allEndpoints) {
        auto entity = m_sessionManager->getPlayerEntity(endpoint, true); // 允许掉线的
        if (entity) {
            allPlayers.push_back(*entity);
        }
    }
    
    m_context.logger->info("准备清理 {} 个玩家实体", allPlayers.size());
    
    // 清理会话
    m_sessionManager->clearAllSessions();
    
    // 销毁实体
    for (auto entity : allPlayers) {
        m_context.registry.destroy(entity);
    }
    
    // 重置游戏状态
    m_gameRunning = false;
    m_context.logger->info("游戏清理完成");
}
```

## 可选：Component 标记掉线状态

除了在 SessionManager 中标记，还可以在 ECS 中添加组件：

```cpp
// 定义掉线标记组件
struct DisconnectedTag {
    std::chrono::steady_clock::time_point disconnectedTime;
};

// 玩家掉线时
void onPlayerDisconnected(entt::entity player) {
    m_context.registry.emplace<DisconnectedTag>(player, std::chrono::steady_clock::now());
}

// 玩家重连时
void onPlayerReconnected(entt::entity player) {
    m_context.registry.remove<DisconnectedTag>(player);
}

// 查询掉线玩家
auto disconnectedPlayers = m_context.registry.view<DisconnectedTag>();
for (auto [entity, tag] : disconnectedPlayers.each()) {
    auto elapsed = std::chrono::steady_clock::now() - tag.disconnectedTime;
    logger->info("玩家 {} 已掉线 {}s", static_cast<uint32_t>(entity), elapsed.count());
}
```

## 优势

1. **断线重连**: 玩家可以在游戏中恢复进度
2. **状态保留**: 掉线期间的游戏状态（手牌、装备等）不会丢失
3. **游戏连续性**: 其他玩家可以继续游戏，不受单个玩家掉线影响
4. **灵活处理**: 可以自动跳过掉线玩家或 AI 托管
5. **统一清理**: 游戏结束时批量清理，避免碎片化

## 注意事项

1. **内存占用**: 掉线会话仍占用内存，需控制会话数量上限
2. **超时时间**: `HEARTBEAT_TIMEOUT` 默认 30 秒，根据游戏节奏调整
3. **状态同步**: 重连时需发送完整游戏状态给客户端
4. **掉线策略**: 根据游戏类型决定是否托管、跳过或等待
5. **安全检查**: 处理游戏消息时始终检查 `isPlayerDisconnected()`

## 总结

通过 **标记掉线而非移除** 的策略，实现了：

✅ 断线重连支持  
✅ 游戏状态持久化  
✅ 灵活的掉线处理策略  
✅ 游戏连续性保证  
✅ 统一的资源管理  

这种设计特别适合回合制卡牌游戏，允许玩家在网络波动或短暂断开后恢复游戏。
