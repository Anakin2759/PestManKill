# 聊天系统使用指南

## 概述

本文档说明如何在客户端和服务器端实现和使用聊天消息功能，包括发送者识别。

## 消息结构

### 客户端请求 (SendMessageToChatRequest)

```cpp
// src/shared/messages/request/SendMessageToChatRequest.h
struct SendMessageToChatRequest
{
    uint32_t sender;        // 发送者的玩家实体 ID
    std::string chatMessage; // 聊天消息内容
};
```

### 服务器响应 (SendMessageToChatResponse)

```cpp
// src/shared/messages/response/SendMessageToChatResponse.h
struct SendMessageToChatResponse
{
    uint32_t sender;        // 发送者的玩家实体 ID
    std::string chatMessage; // 聊天消息内容
};
```

### 客户端事件 (SendMessageToChatResponded)

```cpp
// src/client/events/SendMessageToChatResponded.h
struct SendMessageToChatResponded
{
    uint32_t sender;        // 发送者的玩家实体 ID
    std::string senderName; // 发送者的玩家名称
    std::string message;    // 聊天消息内容
};
```

## 客户端实现

### 1. 发送聊天消息

```cpp
#include "src/client/net/GameClient.h"

auto gameClient = std::make_shared<GameClient>();

// 连接并登录后...
asio::co_spawn(
    utils::ThreadPool::getInstance().get_executor(),
    [gameClient]() -> asio::awaitable<void> {
        co_await gameClient->sendChatMessage("Hello, everyone!");
    },
    asio::detached);
```

### 2. 接收聊天消息（通过 MessageHandler）

```cpp
#include "src/client/net/IServerMessageHandler.h"
#include "src/client/utils/Logger.h"

class MyChatHandler {
public:
    void onBroadcastEvent(const std::string& message) {
        // 消息格式: "[Player 123]: Hello, everyone!"
        utils::LOG_INFO("收到消息: {}", message);
        
        // 解析发送者 ID
        size_t start = message.find("[Player ");
        size_t end = message.find("]:");
        if (start != std::string::npos && end != std::string::npos) {
            std::string playerIdStr = message.substr(start + 8, end - start - 8);
            uint32_t playerId = std::stoul(playerIdStr);
            
            std::string chatMessage = message.substr(end + 3);
            
            utils::LOG_INFO("玩家 {} 说: {}", playerId, chatMessage);
        }
    }
};

// 设置消息处理器
gameClient->setMessageHandler(entt::poly<IServerMessageHandler>{MyChatHandler{}});
```

### 3. 使用事件分发器（推荐）

如果要使用 `entt::dispatcher` 进行更灵活的事件处理：

```cpp
#include <entt/entt.hpp>
#include "src/client/events/SendMessageToChatResponded.h"
#include "src/client/utils/Logger.h"

class ChatEventListener {
public:
    ChatEventListener(entt::dispatcher& dispatcher) {
        // 订阅聊天消息事件
        dispatcher.sink<SendMessageToChatResponded>()
            .connect<&ChatEventListener::onChatMessage>(this);
    }
    
    void onChatMessage(const SendMessageToChatResponded& event) {
        utils::LOG_INFO("收到来自玩家 {} ({}) 的消息: {}", 
            event.sender, 
            event.senderName.empty() ? "Unknown" : event.senderName, 
            event.message);
        
        // 更新 UI 显示聊天消息
        updateChatUI(event);
    }
    
private:
    void updateChatUI(const SendMessageToChatResponded& event) {
        // 在聊天窗口显示消息
        // chatWidget->appendText(event.senderName + ": " + event.message + "\n");
    }
};

// 使用示例
entt::dispatcher dispatcher;
ChatEventListener listener(dispatcher);

// 当收到聊天消息时触发事件
SendMessageToChatResponded event{
    .sender = 123,
    .senderName = "Alice",
    .message = "Hello, world!"
};
dispatcher.trigger(event);
```

### 4. 在 GameClient 中集成事件分发器

修改 `GameClient::handleChatMessage` 以支持事件分发：

```cpp
// 在 GameClient 类中添加
class GameClient {
private:
    std::shared_ptr<entt::dispatcher> m_dispatcher;
    std::unordered_map<uint32_t, std::string> m_playerNames; // 缓存玩家名称

public:
    void setDispatcher(std::shared_ptr<entt::dispatcher> dispatcher) {
        m_dispatcher = dispatcher;
    }
    
    void handleChatMessage(const uint8_t* data, size_t size) {
        try {
            std::string jsonStr(reinterpret_cast<const char*>(data), size);
            nlohmann::json json = nlohmann::json::parse(jsonStr);

            uint32_t sender = json.value("sender", 0u);
            std::string chatMessage = json.value("chatMessage", "");
            
            // 创建事件
            SendMessageToChatResponded event;
            event.sender = sender;
            event.message = chatMessage;
            
            // 查找发送者名称（如果已缓存）
            auto it = m_playerNames.find(sender);
            if (it != m_playerNames.end()) {
                event.senderName = it->second;
            } else {
                event.senderName = "Player " + std::to_string(sender);
            }
            
            // 触发事件
            if (m_dispatcher) {
                m_dispatcher->trigger(event);
            }
            
            // 向后兼容：也调用 message handler
            if (m_messageHandler) {
                std::string formattedMessage = event.senderName + ": " + chatMessage;
                m_messageHandler->onBroadcastEvent(formattedMessage);
            }
        }
        catch (const std::exception& e) {
            utils::LOG_ERROR("Failed to handle chat message: {}", e.what());
        }
    }
};
```

## 服务器实现

### 服务器端处理聊天消息

服务器需要：

1. 接收客户端的 `SendMessageToChatRequest`
2. 验证发送者身份
3. 广播 `SendMessageToChatResponse` 给所有在线玩家（或房间内玩家）

```cpp
// 伪代码示例（服务器端）
void handleChatMessageRequest(const asio::ip::udp::endpoint& sender, 
                              const uint8_t* data, size_t size) {
    try {
        // 解析请求
        std::string jsonStr(reinterpret_cast<const char*>(data), size);
        nlohmann::json json = nlohmann::json::parse(jsonStr);
        
        uint32_t senderId = json.value("sender", 0u);
        std::string message = json.value("chatMessage", "");
        
        // 验证发送者（确保 sender 与连接的玩家实体匹配）
        auto playerEntity = m_sessionManager->getPlayerEntity(sender);
        if (!playerEntity.has_value() || playerEntity.value() != static_cast<entt::entity>(senderId)) {
            LOG_WARN("Chat message sender mismatch");
            return;
        }
        
        // 获取发送者名称
        auto& metaInfo = m_registry.get<MetaPlayerInfo>(playerEntity.value());
        
        // 构造响应
        nlohmann::json response;
        response["sender"] = senderId;
        response["senderName"] = metaInfo.playerName; // 可选：包含名称
        response["chatMessage"] = message;
        
        std::string responseStr = response.dump();
        std::vector<uint8_t> payload(responseStr.begin(), responseStr.end());
        
        // 广播给所有玩家
        auto endpoints = m_sessionManager->getAllEndpoints(true); // 排除掉线玩家
        for (const auto& endpoint : endpoints) {
            asio::co_spawn(
                m_threadPool->get_executor(),
                [this, endpoint, payload]() -> asio::awaitable<void> {
                    co_await m_networkManager->sendReliablePacket(
                        endpoint,
                        static_cast<uint16_t>(MessageType::CHAT_MESSAGE),
                        payload);
                },
                asio::detached);
        }
        
        LOG_INFO("Chat message from {} broadcast to {} players", 
                 metaInfo.playerName, endpoints.size());
    }
    catch (const std::exception& e) {
        LOG_ERROR("Failed to handle chat message: {}", e.what());
    }
}
```

## 完整示例：聊天客户端

### UI 集成

```cpp
#include "src/client/ui/TextEdit.h"
#include "src/client/ui/Button.h"
#include "src/client/ui/Layout.h"
#include "src/client/net/GameClient.h"
#include "src/client/events/SendMessageToChatResponded.h"

class ChatUI {
public:
    ChatUI(std::shared_ptr<GameClient> gameClient, entt::dispatcher& dispatcher)
        : m_gameClient(gameClient)
        , m_dispatcher(dispatcher) {
        setupUI();
        
        // 订阅聊天事件
        m_dispatcher.sink<SendMessageToChatResponded>()
            .connect<&ChatUI::onChatMessage>(this);
    }
    
    void setupUI() {
        // 聊天记录显示
        m_chatDisplay = std::make_shared<ui::TextEdit>();
        m_chatDisplay->setReadOnly(true);
        m_chatDisplay->setFixedSize(500.0f, 400.0f);
        
        // 输入框
        m_chatInput = std::make_shared<ui::TextEdit>();
        m_chatInput->setPlaceholder("输入消息...");
        m_chatInput->setFixedSize(400.0f, 60.0f);
        m_chatInput->setOnTextChanged([this](const std::string& text) {
            if (text.find('\n') != std::string::npos) {
                sendMessage();
            }
        });
        
        // 发送按钮
        auto sendButton = std::make_shared<ui::Button>("发送", [this]() {
            sendMessage();
        });
    }
    
    void onChatMessage(const SendMessageToChatResponded& event) {
        // 格式化并显示消息
        std::string displayName = event.senderName.empty() 
            ? ("Player " + std::to_string(event.sender))
            : event.senderName;
            
        std::string formattedMsg = displayName + ": " + event.message + "\n";
        m_chatDisplay->appendText(formattedMsg);
    }
    
    void sendMessage() {
        std::string message = m_chatInput->getText();
        if (message.empty()) return;
        
        // 移除换行符
        while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
            message.pop_back();
        }
        
        if (!message.empty()) {
            // 异步发送
            asio::co_spawn(
                utils::ThreadPool::getInstance().get_executor(),
                [client = m_gameClient, message]() -> asio::awaitable<void> {
                    co_await client->sendChatMessage(message);
                },
                asio::detached);
            
            m_chatInput->clear();
        }
    }

private:
    std::shared_ptr<GameClient> m_gameClient;
    entt::dispatcher& m_dispatcher;
    std::shared_ptr<ui::TextEdit> m_chatDisplay;
    std::shared_ptr<ui::TextEdit> m_chatInput;
};
```

## 消息格式说明

### JSON 格式（推荐）

**请求:**

```json
{
    "sender": 123,
    "chatMessage": "Hello, world!"
}
```

**响应:**

```json
{
    "sender": 123,
    "senderName": "Alice",
    "chatMessage": "Hello, world!"
}
```

### 纯文本格式（兼容旧版）

服务器也可以发送纯文本格式：

```
[Player 123]: Hello, world!
```

客户端的 `handleChatMessage` 会自动检测并处理两种格式。

## 最佳实践

1. **发送者验证**: 服务器必须验证请求中的 `sender` 字段与连接的玩家实体匹配
2. **名称缓存**: 客户端应缓存玩家名称映射，避免每次都显示 "Player 123"
3. **消息过滤**: 服务器应检查消息内容（长度、敏感词等）
4. **房间隔离**: 聊天消息应只广播给同一房间的玩家
5. **离线处理**: 掉线玩家不应收到聊天消息
6. **消息历史**: 考虑保存最近的聊天记录，供重连玩家查看

## 故障排查

### 问题：无法识别发送者

**解决**: 确保服务器在 `SendMessageToChatResponse` 中包含 `sender` 字段

### 问题：显示 "Player 123" 而不是真实名称

**解决**:

- 服务器在响应中添加 `senderName` 字段
- 客户端维护玩家 ID 到名称的映射表

### 问题：JSON 解析失败

**解决**: 检查服务器发送的数据格式是否正确，确保是有效的 JSON

## 参考

- `src/shared/messages/request/SendMessageToChatRequest.h`
- `src/shared/messages/response/SendMessageToChatResponse.h`
- `src/client/events/SendMessageToChatResponded.h`
- `src/client/net/GameClient.h` - `handleChatMessage()` 方法
- `docs/TextEdit_Usage_Example.md` - TextEdit 组件使用示例
