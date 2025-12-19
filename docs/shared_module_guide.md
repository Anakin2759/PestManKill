# Shared 模块使用指南

## 概述

Shared 模块提供了客户端和服务端共享的消息定义、协议接口和工具类。经过重构，现已完全适配 **Net 模块**的帧协议和序列化接口。

---

## 核心组件

### 1. FrameHeader（帧头定义）

位置：`src/shared/messages/FrameHeader.h`

```cpp
#pragma pack(push, 1)
struct FrameHeader {
    uint16_t magic = 0x55AA;  // 固定魔数
    uint16_t cmd = 0;         // 命令ID
    uint16_t length = 0;      // 载荷长度
};
#pragma pack(pop)
```

**命令ID命名空间**：

```cpp
namespace CommandID {
    // 连接与认证 (0x1000 - 0x1FFF)
    constexpr uint16_t CONNECTED = 0x1001;
    constexpr uint16_t CONNECTED_ACK = 0x1002;
    constexpr uint16_t HEARTBEAT = 0x1010;
    
    // 房间管理 (0x2000 - 0x2FFF)
    constexpr uint16_t CREATE_ROOM_REQ = 0x2001;
    constexpr uint16_t CREATE_ROOM_RESP = 0x2002;
    
    // 聊天系统 (0x3000 - 0x3FFF)
    constexpr uint16_t SEND_MESSAGE_REQ = 0x3001;
    constexpr uint16_t SEND_MESSAGE_RESP = 0x3002;
    
    // 游戏逻辑 (0x4000 - 0x4FFF)
    constexpr uint16_t USE_CARD_REQ = 0x4001;
    constexpr uint16_t DISCARD_CARD_REQ = 0x4010;
    
    // 游戏状态同步 (0x5000 - 0x5FFF)
    constexpr uint16_t GAME_STATE_SYNC = 0x5001;
    
    // 错误与通知 (0xF000 - 0xFFFF)
    constexpr uint16_t ERROR_NOTIFY = 0xF001;
}
```

**辅助函数**：

```cpp
bool isRequest(uint16_t cmd);        // 判断是否为请求（奇数）
bool isResponse(uint16_t cmd);       // 判断是否为响应（偶数）
uint16_t getResponseCmd(uint16_t);   // 获取对应的响应命令ID
```

---

### 2. MessageBase（消息基类）

位置：`src/shared/messages/MessageBase.h`

提供统一的消息序列化/反序列化接口（CRTP 模式）。

**接口定义**：

```cpp
template <typename Derived>
class MessageBase {
public:
    // 二进制序列化
    std::expected<std::span<uint8_t>, MessageError> 
    serialize(std::span<uint8_t> buffer) const;
    
    // 二进制反序列化
    static std::expected<Derived, MessageError> 
    deserialize(std::span<const uint8_t> data);
    
    // JSON 序列化
    nlohmann::json toJson() const;
    
    // JSON 反序列化
    static std::expected<Derived, MessageError> 
    fromJson(const nlohmann::json& json);
};
```

**错误类型**：

```cpp
enum class MessageError {
    SerializeFailed,
    DeserializeFailed,
    InvalidFormat,
    BufferTooSmall
};
```

---

### 3. 消息定义示例

#### ConnectedRequest（连接请求）

```cpp
struct ConnectedRequest : public MessageBase<ConnectedRequest>
{
    static constexpr uint16_t CMD_ID = CommandID::CONNECTED;
    static constexpr size_t MAX_PLAYER_NAME_LEN = 32;

    char playerName[MAX_PLAYER_NAME_LEN] = {};
    uint32_t clientVersion = 1;

    // 便捷构造
    static ConnectedRequest create(const std::string& name, uint32_t version = 1);

    // 实现序列化接口
    std::expected<std::span<uint8_t>, MessageError> 
    serializeImpl(std::span<uint8_t> buffer) const;
    
    static std::expected<ConnectedRequest, MessageError> 
    deserializeImpl(std::span<const uint8_t> data);
    
    nlohmann::json toJsonImpl() const;
    static std::expected<ConnectedRequest, MessageError> 
    fromJsonImpl(const nlohmann::json& j);
};
```

#### CreateRoomRequest/Response（创建房间）

```cpp
struct CreateRoomRequest : public MessageBase<CreateRoomRequest>
{
    static constexpr uint16_t CMD_ID = CommandID::CREATE_ROOM_REQ;
    
    char roomName[64] = {};
    uint32_t maxPlayers = 5;
    uint8_t gameMode = 0;
    
    static CreateRoomRequest create(const std::string& name, uint32_t max, uint8_t mode = 0);
    // ... 实现序列化接口
};

struct CreateRoomResponse : public MessageBase<CreateRoomResponse>
{
    static constexpr uint16_t CMD_ID = CommandID::CREATE_ROOM_RESP;
    
    uint32_t roomId = 0;
    uint8_t success = 0;
    uint8_t errorCode = 0;
    
    static CreateRoomResponse createSuccess(uint32_t roomId);
    static CreateRoomResponse createFailed(uint8_t errorCode);
    // ... 实现序列化接口
};
```

---

### 4. MessageDispatcher（消息分发器）

位置：`src/shared/messages/MessageDispatcher.h`

用于注册和分发消息处理器。

**使用示例**：

```cpp
#include "src/shared/messages/MessageDispatcher.h"
#include "src/shared/messages/request/CreateRoomRequest.h"
#include "src/shared/messages/response/CreateRoomResponse.h"

MessageDispatcher dispatcher;

// 注册处理器
dispatcher.registerHandler<CreateRoomRequest>([](const CreateRoomRequest& req) 
    -> std::expected<std::vector<uint8_t>, MessageError> 
{
    // 业务逻辑：创建房间
    uint32_t roomId = createRoom(req.roomName, req.maxPlayers);
    
    // 构造响应
    auto resp = CreateRoomResponse::createSuccess(roomId);
    
    // 序列化响应
    std::vector<uint8_t> buffer(256);
    auto data = resp.serialize(buffer);
    if (!data) {
        return std::unexpected(data.error());
    }
    
    return std::vector<uint8_t>(data->begin(), data->end());
});

// 分发消息
auto frameResult = decodeFrame(receivedData);
if (frameResult) {
    auto [cmd, payload] = *frameResult;
    auto response = dispatcher.dispatch(cmd, payload);
    
    if (response) {
        // 发送响应
        session->send(*response);
    }
}
```

---

## 完整使用流程

### 服务端消息处理

```cpp
#include "src/net/App/Server.h"
#include "src/shared/messages/MessageDispatcher.h"
#include "src/shared/messages/request/CreateRoomRequest.h"
#include "src/shared/messages/request/ConnectedRequest.h"

class GameServer : public Server
{
public:
    GameServer(IUdpTransport& transport, asio::any_io_executor exec, size_t threads)
        : Server(transport, exec, threads)
    {
        setupMessageHandlers();
    }

private:
    void setupMessageHandlers()
    {
        // 注册连接请求处理器
        m_dispatcher.registerHandler<ConnectedRequest>([this](const ConnectedRequest& req) {
            // 处理连接
            auto playerId = registerPlayer(req.playerName);
            
            // 构造响应（假设有 ConnectedResponse）
            // ...
            return encodeMessage(response);
        });

        // 注册创建房间处理器
        m_dispatcher.registerHandler<CreateRoomRequest>([this](const CreateRoomRequest& req) {
            auto roomId = createGameRoom(req.roomName, req.maxPlayers, req.gameMode);
            
            auto resp = roomId > 0 
                ? CreateRoomResponse::createSuccess(roomId)
                : CreateRoomResponse::createFailed(1); // 错误码1：创建失败
            
            return encodeMessage(resp);
        });
    }

    void onSession(uint32_t conv, std::shared_ptr<KcpSession> session) override
    {
        // 启动玩家协程
        auto executor = asio::make_strand(m_pool.get_executor());
        asio::co_spawn(executor, handlePlayer(conv, session), asio::detached);
    }

    asio::awaitable<void> handlePlayer(uint32_t conv, std::shared_ptr<KcpSession> session)
    {
        while (true) {
            // 接收 KCP 数据
            auto kcpData = co_await session->recv();
            if (!kcpData) break;

            // 解码帧
            auto frameResult = decodeFrame(*kcpData);
            if (!frameResult) continue;

            auto [cmd, payload] = *frameResult;

            // 分发消息
            auto response = m_dispatcher.dispatch(cmd, payload);
            if (response) {
                // 编码为帧并发送
                std::vector<uint8_t> frameBuffer(2048);
                auto responseCmd = getResponseCmd(cmd);
                auto frameData = encodeFrame(frameBuffer, responseCmd, *response);
                
                if (frameData) {
                    session->send(*frameData);
                }
            }
        }
    }

    MessageDispatcher m_dispatcher;
};
```

---

### 客户端发送消息

```cpp
#include "src/net/App/Client.h"
#include "src/shared/messages/MessageDispatcher.h"
#include "src/shared/messages/request/CreateRoomRequest.h"

asio::awaitable<void> clientRoutine(std::shared_ptr<KcpSession> session)
{
    // 1. 构造请求
    auto req = CreateRoomRequest::create("MyRoom", 5, 0);

    // 2. 编码消息（包含帧头）
    auto encodedMsg = encodeMessage(req);
    if (!encodedMsg) {
        co_return;
    }

    // 3. 发送
    session->send(*encodedMsg);

    // 4. 接收响应
    auto kcpData = co_await session->recv();
    if (!kcpData) {
        co_return;
    }

    // 5. 解码响应
    auto resp = decodeMessage<CreateRoomResponse>(*kcpData);
    if (resp && resp->success) {
        std::cout << "Room created with ID: " << resp->roomId << std::endl;
    }
}
```

---

## 自定义消息定义

### 步骤1：定义消息结构

```cpp
// src/shared/messages/request/MyCustomRequest.h
#pragma once
#include "../MessageBase.h"
#include "../FrameHeader.h"

struct MyCustomRequest : public MessageBase<MyCustomRequest>
{
    static constexpr uint16_t CMD_ID = 0x4100; // 自定义命令ID

    uint32_t field1 = 0;
    uint64_t field2 = 0;
    char field3[32] = {};

    // 二进制序列化
    std::expected<std::span<uint8_t>, MessageError> 
    serializeImpl(std::span<uint8_t> buffer) const {
        if (buffer.size() < sizeof(MyCustomRequest)) {
            return std::unexpected(MessageError::BufferTooSmall);
        }
        std::memcpy(buffer.data(), this, sizeof(MyCustomRequest));
        return buffer.subspan(0, sizeof(MyCustomRequest));
    }

    static std::expected<MyCustomRequest, MessageError> 
    deserializeImpl(std::span<const uint8_t> data) {
        if (data.size() < sizeof(MyCustomRequest)) {
            return std::unexpected(MessageError::InvalidFormat);
        }
        MyCustomRequest msg;
        std::memcpy(&msg, data.data(), sizeof(MyCustomRequest));
        return msg;
    }

    // JSON 序列化
    nlohmann::json toJsonImpl() const {
        return {
            {"field1", field1},
            {"field2", field2},
            {"field3", std::string(field3)}
        };
    }

    static std::expected<MyCustomRequest, MessageError> 
    fromJsonImpl(const nlohmann::json& j) {
        try {
            MyCustomRequest msg;
            msg.field1 = j.at("field1").get<uint32_t>();
            msg.field2 = j.at("field2").get<uint64_t>();
            std::strncpy(msg.field3, j.at("field3").get<std::string>().c_str(), 31);
            return msg;
        } catch (...) {
            return std::unexpected(MessageError::DeserializeFailed);
        }
    }
};
```

### 步骤2：注册处理器

```cpp
dispatcher.registerHandler<MyCustomRequest>([](const MyCustomRequest& req) {
    // 处理逻辑
    // ...
    return encodeMessage(response);
});
```

---

## 最佳实践

### 1. 消息设计原则

- ✅ **固定长度优先**：使用固定大小的字符数组而非 `std::string`
- ✅ **内存对齐**：使用 `reserved` 字段对齐到4/8字节
- ✅ **版本控制**：预留版本字段，便于协议升级
- ✅ **错误码规范**：统一定义错误码枚举

### 2. 命令ID分配

```cpp
// 按模块划分命令ID段
0x1000 - 0x1FFF: 连接与认证
0x2000 - 0x2FFF: 房间管理
0x3000 - 0x3FFF: 聊天系统
0x4000 - 0x4FFF: 游戏逻辑（卡牌操作）
0x5000 - 0x5FFF: 游戏状态同步
0xF000 - 0xFFFF: 错误与通知

// 请求/响应配对
请求命令ID（奇数）：0xXXX1, 0xXXX3, 0xXXX5
响应命令ID（偶数）：0xXXX2, 0xXXX4, 0xXXX6
```

### 3. 性能优化

- **复用缓冲区**：避免频繁分配 `std::vector`

  ```cpp
  std::vector<uint8_t> reusableBuffer(2048);
  auto data = message.serialize(reusableBuffer);
  ```

- **批量发送**：多个小消息合并为一个 KCP 包

  ```cpp
  std::vector<uint8_t> batch;
  for (auto& msg : messages) {
      auto encoded = encodeMessage(msg);
      batch.insert(batch.end(), encoded->begin(), encoded->end());
  }
  session->send(batch);
  ```

- **避免拷贝**：使用 `std::span` 传递数据视图

### 4. 错误处理

```cpp
// 总是检查 std::expected 返回值
auto result = message.serialize(buffer);
if (!result) {
    switch (result.error()) {
        case MessageError::BufferTooSmall:
            // 扩大缓冲区
            break;
        case MessageError::SerializeFailed:
            // 记录日志
            break;
    }
}
```

---

## 与 Net 模块集成

### 架构图

```
┌─────────────────────────────────────────────┐
│         Application Layer                   │
│  (GameServer / GameClient Logic)            │
└─────────────────┬───────────────────────────┘
                  │
        ┌─────────▼──────────┐
        │ MessageDispatcher  │ ← shared/messages/
        └─────────┬──────────┘
                  │
     ┌────────────▼────────────┐
     │  encodeMessage()        │ ← FrameCodec + MessageBase
     │  decodeMessage()        │
     └────────────┬────────────┘
                  │
        ┌─────────▼──────────┐
        │  KcpSession        │ ← net/Session/
        │  (recv/send)       │
        └─────────┬──────────┘
                  │
        ┌─────────▼──────────┐
        │  KcpEndpoint       │ ← net/App/
        │  (input/update)    │
        └─────────┬──────────┘
                  │
        ┌─────────▼──────────┐
        │  AsioUdpTransport  │ ← net/transport/
        └────────────────────┘
```

### 数据流向

**发送流程**：

```
业务对象 → MessageBase::serialize() → FrameCodec::encodeFrame() 
→ KcpSession::send() → KCP 协议处理 → AsioUdpTransport::send() → UDP
```

**接收流程**：

```
UDP → AsioUdpTransport::recvLoop() → KcpEndpoint::input() 
→ KcpSession::input() → KcpSession::recv() → FrameCodec::decodeFrame() 
→ MessageBase::deserialize() → MessageDispatcher::dispatch() → 业务处理器
```

---

## 测试

### 单元测试示例

```cpp
#include <gtest/gtest.h>
#include "src/shared/messages/request/CreateRoomRequest.h"

TEST(CreateRoomRequestTest, Serialization) {
    auto req = CreateRoomRequest::create("TestRoom", 8, 1);
    
    std::vector<uint8_t> buffer(256);
    auto result = req.serialize(buffer);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_GT(result->size(), 0);
}

TEST(CreateRoomRequestTest, Deserialization) {
    auto req1 = CreateRoomRequest::create("TestRoom", 8, 1);
    
    std::vector<uint8_t> buffer(256);
    auto serialized = req1.serialize(buffer);
    ASSERT_TRUE(serialized.has_value());
    
    auto req2 = CreateRoomRequest::deserialize(*serialized);
    ASSERT_TRUE(req2.has_value());
    
    EXPECT_STREQ(req1.roomName, req2->roomName);
    EXPECT_EQ(req1.maxPlayers, req2->maxPlayers);
    EXPECT_EQ(req1.gameMode, req2->gameMode);
}

TEST(CreateRoomRequestTest, JsonSerialization) {
    auto req1 = CreateRoomRequest::create("TestRoom", 8, 1);
    
    auto json = req1.toJson();
    auto req2 = CreateRoomRequest::fromJson(json);
    
    ASSERT_TRUE(req2.has_value());
    EXPECT_STREQ(req1.roomName, req2->roomName);
}
```

---

## 常见问题

### Q: 为什么使用固定长度字符数组而非 std::string？

**A**:

- 性能：避免堆分配，提高序列化速度
- 可预测性：消息大小固定，便于内存管理
- 网络友好：直接 memcpy，无需额外编码长度信息

### Q: 如何处理大数据（如聊天消息、文件）？

**A**: 使用变长消息格式：

```cpp
struct LargeDataMessage {
    uint32_t dataLength;
    // 后续动态数据通过 payload 传递
};
```

### Q: 如何实现消息压缩？

**A**: 在序列化后、编码前压缩：

```cpp
auto serialized = message.serialize(buffer);
auto compressed = compress(*serialized); // 自定义压缩函数
auto frame = encodeFrame(frameBuffer, cmd, compressed);
```

---

## 作者

**AnakinLiu** (<azrael2759@qq.com>)  
版本: 0.2  
日期: 2025-12-18
