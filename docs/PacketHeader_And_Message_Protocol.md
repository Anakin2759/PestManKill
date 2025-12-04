# PacketHeader 与 Message 的组合发送机制

## 概览

PestManKill 使用自定义的 UDP 数据包协议，每个数据包由 **PacketHeader（包头）** + **Payload（消息体）** 组成。这种设计将网络传输层和应用层解耦，实现了可靠传输、消息类型识别和序列化的完整解决方案。

## 数据包结构

### 整体布局

```
┌─────────────────────────────────────────────────────────┐
│                    UDP Packet (Max 1024 bytes)          │
├─────────────────────────────────────────────────────────┤
│  PacketHeader (12 bytes)                                │
│  ┌─────────────────────────────────────────────────┐   │
│  │ seq (4 bytes)   | 序列号                        │   │
│  │ ack (4 bytes)   | 确认序列号                    │   │
│  │ type (2 bytes)  | 消息类型 (MessageType)        │   │
│  │ size (2 bytes)  | Payload 大小                  │   │
│  └─────────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────────┤
│  Payload (0-1012 bytes)                                 │
│  ┌─────────────────────────────────────────────────┐   │
│  │ JSON 序列化的消息内容 (UseCardMessage, etc.)   │   │
│  │ 或其他二进制数据                                │   │
│  └─────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

### PacketHeader 定义

**文件**: `src/shared/messages/PacketHeader.h`

```cpp
struct PacketHeader
{
    uint32_t seq;   // 序列号（用于可靠传输，0 表示不可靠消息）
    uint32_t ack;   // 确认序列号（ACK 响应时填充）
    uint16_t type;  // 消息类型（对应 MessageType 枚举）
    uint16_t size;  // payload 字节大小
};
```

**字段说明**:

| 字段 | 大小 | 说明 |
|------|------|------|
| `seq` | 4 字节 | 可靠消息的序列号，0 表示不可靠消息 |
| `ack` | 4 字节 | ACK 响应包中填充被确认的 seq |
| `type` | 2 字节 | 消息类型 (如 `MessageType::USE_CARD = 0x0100`) |
| `size` | 2 字节 | Payload 的实际字节数 (不包括 header) |

### MessageType 枚举

**文件**: `src/shared/common/Common.h`

```cpp
enum class MessageType : uint16_t
{
    HEARTBEAT = 0x0001,      // 心跳包
    LOGIN,                   // 登录请求
    LOGOUT,                  // 登出请求
    USE_CARD = 0x0100,       // 使用卡牌
    DRAW_CARD,               // 抽卡
    DISCARD_CARD,            // 弃牌
    END_TURN,                // 结束回合
    CHAT_MESSAGE = 0x0200,   // 聊天消息
    GAME_STATE = 0x0300,     // 游戏状态同步
    ERROR_MESSAGE = 0x0F00,  // 错误消息
    ACK = 0xFFFF             // 确认包
};
```

## 发送流程

### 1. 发送端构造数据包

#### 步骤 1: 准备 Message Payload

**示例**: `UseCardMessage`

**文件**: `src/shared/messages/UseCardMessage.h`

```cpp
struct UseCardMessage
{
    uint32_t player;                  // 玩家实体 ID
    uint32_t card;                    // 卡牌实体 ID
    std::vector<uint32_t> targets;    // 目标实体 ID 列表

    // 序列化为 JSON
    [[nodiscard]] nlohmann::json toJson() const {
        return {
            {"player", player}, 
            {"card", card}, 
            {"targets", targets}
        };
    }

    // 从 JSON 反序列化
    static UseCardMessage fromJson(const nlohmann::json& json) {
        return {
            .player = json.at("player").get<uint32_t>(),
            .card = json.at("card").get<uint32_t>(),
            .targets = json.at("targets").get<std::vector<uint32_t>>()
        };
    }
};
```

**序列化示例**:

```cpp
UseCardMessage msg{
    .player = 5,
    .card = 42,
    .targets = {7, 8}
};

// JSON 结果: {"player":5,"card":42,"targets":[7,8]}
std::string jsonStr = msg.toJson().dump();
std::vector<uint8_t> payload(jsonStr.begin(), jsonStr.end());
```

#### 步骤 2: 构建完整数据包

**函数**: `NetWorkManager::buildPacket()`

**文件**: `src/server/net/NetWorkManager.h`

```cpp
static std::shared_ptr<std::vector<uint8_t>>
buildPacket(uint16_t type, uint32_t seq, const std::vector<uint8_t>& payload)
{
    // 分配内存：Header + Payload
    auto data = std::make_shared<std::vector<uint8_t>>(
        sizeof(PacketHeader) + payload.size()
    );
    
    // 填充 Header
    PacketHeader header{
        .seq = seq,                                   // 序列号
        .ack = 0,                                     // 非 ACK 包
        .type = type,                                 // 消息类型
        .size = static_cast<uint16_t>(payload.size()) // Payload 大小
    };
    
    // 复制 Header 到数据包前 12 字节
    std::memcpy(data->data(), &header, sizeof(PacketHeader));
    
    // 复制 Payload 到 Header 后面
    if (!payload.empty()) {
        std::memcpy(
            &(*data)[sizeof(PacketHeader)],  // 目标地址：Header 之后
            payload.data(),                   // 源地址：Payload
            payload.size()                    // 大小
        );
    }
    
    return data;
}
```

**内存布局示例**:

假设发送 `UseCardMessage{player:5, card:42, targets:[7,8]}`

```
JSON String: {"player":5,"card":42,"targets":[7,8]}
Payload 大小: 38 字节

数据包内存布局 (50 字节总大小):
┌────────────────────────────────────────────────────────┐
│ Offset | Content                    | Hex              │
├────────┼────────────────────────────┼──────────────────┤
│ 0-3    | seq = 1                    | 01 00 00 00      │
│ 4-7    | ack = 0                    | 00 00 00 00      │
│ 8-9    | type = 0x0100 (USE_CARD)   | 00 01            │
│ 10-11  | size = 38                  | 26 00            │
├────────┼────────────────────────────┼──────────────────┤
│ 12-49  | JSON: {"player":5,"card... | 7B 22 70 6C ...  │
└────────────────────────────────────────────────────────┘
```

#### 步骤 3: 发送到网络

```cpp
// 不可靠发送
asio::awaitable<void> sendPacket(
    asio::ip::udp::endpoint endpoint, 
    uint16_t type, 
    std::vector<uint8_t> payload)
{
    auto data = buildPacket(type, 0, payload);  // seq = 0 表示不可靠
    
    co_await m_socket->async_send_to(
        asio::buffer(*data),  // 发送整个数据包
        endpoint,
        asio::use_awaitable
    );
}

// 可靠发送（带重传）
asio::awaitable<bool> sendReliablePacket(
    asio::ip::udp::endpoint endpoint,
    uint16_t type,
    std::vector<uint8_t> payload,
    int maxRetries = 3,
    std::chrono::milliseconds timeout = std::chrono::milliseconds(1000))
{
    uint32_t seq = m_nextSeq.fetch_add(1);  // 分配唯一序列号
    auto data = buildPacket(type, seq, payload);
    
    for (int retry = 0; retry < maxRetries; ++retry) {
        co_await m_socket->async_send_to(
            asio::buffer(*data),
            endpoint,
            asio::use_awaitable
        );
        
        // 等待 ACK
        bool ack = co_await waitForAck(seq, endpoint, timeout);
        if (ack) {
            co_return true;  // 成功
        }
        // 超时后重传
    }
    
    co_return false;  // 失败
}
```

### 2. 接收端解析数据包

#### 步骤 1: 接收 UDP 数据包

**函数**: `NetWorkManager::receiveLoop()`

```cpp
asio::awaitable<void> receiveLoop()
{
    std::array<uint8_t, MAX_PACKET_SIZE> buf{};  // 1024 字节缓冲区
    asio::ip::udp::endpoint sender;
    
    while (m_running.load()) {
        // 异步接收
        std::size_t bytes = co_await m_socket->async_receive_from(
            asio::buffer(buf),
            sender,           // 接收发送者地址
            asio::use_awaitable
        );
        
        // 验证最小长度
        if (bytes < sizeof(PacketHeader)) continue;
        
        // 解析 Header
        PacketHeader header{};
        std::memcpy(&header, buf.data(), sizeof(PacketHeader));
        
        // 处理不同类型的包...
    }
}
```

#### 步骤 2: 提取 Header 信息

```cpp
// buf[] 内存布局:
// [0-11]:   PacketHeader
// [12-end]: Payload

PacketHeader header{};
std::memcpy(&header, buf.data(), sizeof(PacketHeader));

// 现在可以访问:
header.seq   // 序列号
header.ack   // 确认序列号
header.type  // 消息类型
header.size  // Payload 大小
```

#### 步骤 3: 处理 ACK 包

```cpp
// 特殊处理：ACK 响应包
if (header.type == ACK_PACKET_TYPE && header.ack != 0) {
    // 这是 ACK 响应，通知等待该 seq 的发送方
    std::shared_ptr<asio::steady_timer> timer;
    AckKey key{header.ack, sender};
    
    {
        std::scoped_lock lock(m_ackMutex);
        auto iter = m_pendingAcks.find(key);
        if (iter != m_pendingAcks.end()) {
            timer = iter->second;
            m_pendingAcks.erase(iter);  // 移除等待
        }
    }
    
    if (timer) {
        timer->cancel();  // 取消超时计时器
    }
    continue;
}
```

#### 步骤 4: 发送 ACK（可靠消息）

```cpp
// 收到可靠包（seq != 0）时自动回复 ACK
if (header.seq != 0) {
    asio::co_spawn(
        m_threadPool->get_executor(),
        [self = shared_from_this(), seq = header.seq, sender]() -> asio::awaitable<void> {
            co_await self->sendAck(seq, sender);
        },
        asio::detached
    );
}

// sendAck 实现
asio::awaitable<void> sendAck(uint32_t seq, asio::ip::udp::endpoint endpoint)
{
    PacketHeader ack{
        .seq = 0,
        .ack = seq,             // 填充被确认的序列号
        .type = ACK_PACKET_TYPE,
        .size = 0               // ACK 无 Payload
    };
    
    std::array<uint8_t, sizeof(PacketHeader)> ackBuf{};
    std::memcpy(ackBuf.data(), &ack, sizeof(PacketHeader));
    
    co_await m_socket->async_send_to(
        asio::buffer(ackBuf),
        endpoint,
        asio::use_awaitable
    );
}
```

#### 步骤 5: 提取 Payload 并调用 Handler

```cpp
// 调用用户注册的 packet handler
if (m_packetHandler) {
    asio::post(
        m_threadPool->get_executor(),
        [self = shared_from_this(), 
         buf, 
         size = header.size, 
         sender, 
         handler = m_packetHandler]() {
            // 传递 Payload 部分（跳过 Header）
            handler(
                &buf[sizeof(PacketHeader)],  // Payload 起始地址
                size,                         // Payload 大小
                sender                        // 发送者地址
            );
        }
    );
}
```

### 3. 应用层处理 Payload

#### 解析 JSON 消息

```cpp
void onPacketReceived(const uint8_t* data, size_t size, const asio::ip::udp::endpoint& sender)
{
    // 假设已知这是 UseCardMessage (通过 header.type 判断)
    
    // 1. 将 bytes 转为 string
    std::string jsonStr(reinterpret_cast<const char*>(data), size);
    
    // 2. 解析 JSON
    nlohmann::json json = nlohmann::json::parse(jsonStr);
    
    // 3. 反序列化为 Message 对象
    UseCardMessage msg = UseCardMessage::fromJson(json);
    
    // 4. 业务逻辑处理
    logger->info("玩家 {} 使用卡牌 {} 目标 [{}]", 
                 msg.player, msg.card, fmt::join(msg.targets, ","));
    
    // 5. 转换为游戏实体
    entt::entity playerEntity = static_cast<entt::entity>(msg.player);
    entt::entity cardEntity = static_cast<entt::entity>(msg.card);
    
    // 6. 触发游戏事件
    dispatcher.trigger<events::UseCardEvent>(playerEntity, cardEntity, msg.targets);
}
```

## 完整示例：客户端发送使用卡牌消息

### 客户端代码

```cpp
#include "src/client/net/NetWorkClient.h"
#include "src/shared/messages/UseCardMessage.h"
#include "src/shared/common/Common.h"

class GameClient {
public:
    asio::awaitable<void> sendUseCard(uint32_t player, uint32_t card, std::vector<uint32_t> targets) {
        // 1. 构造 Message
        UseCardMessage msg{
            .player = player,
            .card = card,
            .targets = targets
        };
        
        // 2. 序列化为 JSON
        std::string jsonStr = msg.toJson().dump();
        std::vector<uint8_t> payload(jsonStr.begin(), jsonStr.end());
        
        // 3. 发送（NetWorkClient 内部会调用 buildPacket）
        bool success = co_await m_networkClient->sendReliablePacket(
            static_cast<uint16_t>(MessageType::USE_CARD),  // type
            payload,                                         // payload
            3,                                               // 重试 3 次
            std::chrono::milliseconds(1000)                 // 超时 1 秒
        );
        
        if (success) {
            m_logger->info("发送使用卡牌消息成功");
        } else {
            m_logger->error("发送使用卡牌消息失败");
        }
    }
};
```

### 服务器代码

```cpp
#include "src/server/net/NetWorkManager.h"
#include "src/shared/messages/UseCardMessage.h"

class GameServer {
public:
    void setupNetworking() {
        m_networkManager->setPacketHandler(
            [this](const uint8_t* data, size_t size, const asio::ip::udp::endpoint& sender) {
                onPacketReceived(data, size, sender);
            }
        );
    }
    
    void onPacketReceived(const uint8_t* data, size_t size, const asio::ip::udp::endpoint& sender) {
        // 注意：这里收到的是 Payload，Header 已被 NetWorkManager 解析
        // 需要通过其他方式传递 header.type（可以修改 handler 签名）
        
        // 假设通过某种方式知道这是 USE_CARD 消息
        std::string jsonStr(reinterpret_cast<const char*>(data), size);
        
        try {
            nlohmann::json json = nlohmann::json::parse(jsonStr);
            UseCardMessage msg = UseCardMessage::fromJson(json);
            
            handleUseCard(msg, sender);
        } catch (const std::exception& e) {
            m_logger->error("解析 UseCardMessage 失败: {}", e.what());
        }
    }
    
    void handleUseCard(const UseCardMessage& msg, const asio::ip::udp::endpoint& sender) {
        // 验证客户端身份
        auto playerEntity = m_sessionManager->getPlayerEntity(sender);
        if (!playerEntity) {
            m_logger->warn("未认证的客户端尝试使用卡牌");
            return;
        }
        
        // 验证玩家实体 ID 匹配
        if (static_cast<uint32_t>(*playerEntity) != msg.player) {
            m_logger->error("玩家实体 ID 不匹配");
            return;
        }
        
        // 处理游戏逻辑
        entt::entity cardEntity = static_cast<entt::entity>(msg.card);
        m_context.dispatcher.trigger<events::UseCardEvent>(*playerEntity, cardEntity, msg.targets);
    }
};
```

## 数据包示例分析

### 示例 1: 不可靠心跳包

**客户端发送**:

```cpp
std::vector<uint8_t> emptyPayload;
co_await networkClient->sendPacket(
    static_cast<uint16_t>(MessageType::HEARTBEAT),
    emptyPayload
);
```

**数据包内容** (12 字节):

```
00 00 00 00   // seq = 0 (不可靠)
00 00 00 00   // ack = 0
01 00         // type = 0x0001 (HEARTBEAT)
00 00         // size = 0 (无 Payload)
```

### 示例 2: 可靠登录消息

**客户端发送**:

```cpp
std::string playerName = "Player_A";
std::vector<uint8_t> payload(playerName.begin(), playerName.end());

co_await networkClient->sendReliablePacket(
    static_cast<uint16_t>(MessageType::LOGIN),
    payload
);
```

**数据包内容** (20 字节):

```
05 00 00 00   // seq = 5 (第 5 个可靠消息)
00 00 00 00   // ack = 0
02 00         // type = 0x0002 (LOGIN)
08 00         // size = 8
50 6C 61 79   // "Play"
65 72 5F 41   // "er_A"
```

**服务器回复 ACK** (12 字节):

```
00 00 00 00   // seq = 0
05 00 00 00   // ack = 5 (确认序列号 5)
FF FF         // type = 0xFFFF (ACK)
00 00         // size = 0
```

### 示例 3: 使用卡牌消息

**客户端发送**:

```cpp
UseCardMessage msg{.player = 5, .card = 42, .targets = {7, 8}};
std::string jsonStr = msg.toJson().dump();
std::vector<uint8_t> payload(jsonStr.begin(), jsonStr.end());

co_await networkClient->sendReliablePacket(
    static_cast<uint16_t>(MessageType::USE_CARD),
    payload
);
```

**JSON Payload**:

```json
{"player":5,"card":42,"targets":[7,8]}
```

**数据包内容** (50 字节):

```
0A 00 00 00   // seq = 10
00 00 00 00   // ack = 0
00 01         // type = 0x0100 (USE_CARD)
26 00         // size = 38 (JSON 长度)
7B 22 70 6C   // {"pl
61 79 65 72   // ayer
22 3A 35 2C   // ":5,
22 63 61 72   // "car
64 22 3A 34   // d":4
32 2C 22 74   // 2,"t
61 72 67 65   // arge
74 73 22 3A   // ts":
5B 37 2C 38   // [7,8
5D 7D         // ]}
```

## 设计优势

### 1. 分层清晰

```
┌─────────────────────────────────────┐
│  应用层 (Message)                   │
│  - UseCardMessage, LoginMessage     │
│  - JSON 序列化/反序列化             │
└─────────────────┬───────────────────┘
                  │
┌─────────────────▼───────────────────┐
│  协议层 (PacketHeader)              │
│  - 序列号管理                       │
│  - 消息类型识别                     │
│  - 可靠传输 (ACK)                   │
└─────────────────┬───────────────────┘
                  │
┌─────────────────▼───────────────────┐
│  传输层 (UDP)                       │
│  - 网络发送/接收                    │
└─────────────────────────────────────┘
```

### 2. 可扩展性

- **新增消息类型**: 只需定义新的 `MessageType` 和对应的 Message 结构体
- **序列化方式**: 可替换为 Protobuf、MessagePack 等
- **可靠性控制**: 通过 `seq` 灵活控制哪些消息需要可靠传输

### 3. 性能优化

- **零拷贝**: `buildPacket` 使用 `std::memcpy` 直接拼接
- **内存池**: 可改造为使用预分配的 buffer pool
- **批量发送**: 可实现多个小消息合并为一个大包

### 4. 调试友好

- **Header 固定大小**: 易于用 Wireshark 等工具分析
- **JSON 可读**: Payload 使用 JSON，便于日志记录和调试
- **类型明确**: `MessageType` 枚举清晰标识每种消息

## 常见问题

### Q1: 为什么不直接发送 Message 对象的二进制？

**A**: 使用 JSON 序列化有以下优势：

- 跨平台兼容性（不同字节序、对齐方式）
- 易于调试和日志记录
- 版本兼容性（可忽略未知字段）

如果需要更高性能，可替换为 Protobuf 或自定义二进制序列化。

### Q2: PacketHeader 的 size 字段有必要吗？

**A**: 有必要！虽然 UDP 本身知道数据包大小，但：

- 可验证数据完整性
- 支持未来的包合并/拆分优化
- 明确 Payload 边界，便于调试

### Q3: 如何处理大于 1024 字节的消息？

**A**: 有两种方案：

1. **分包**: 在应用层将大消息拆分为多个小包
2. **升级为 TCP**: 对于大数据（如游戏状态同步），使用 TCP 连接

### Q4: 为什么 ACK 也需要完整的 PacketHeader？

**A**: 统一格式简化了代码：

- 接收端用同一个函数解析所有包
- 未来可能在 ACK 中携带额外信息
- 12 字节开销在 UDP 中可接受

## 总结

PestManKill 的网络协议通过 **PacketHeader + JSON Payload** 的组合，实现了：

✅ **清晰的分层**: 网络层 (Header) 与应用层 (Message) 解耦  
✅ **可靠传输**: 基于 seq/ack 的重传机制  
✅ **类型安全**: MessageType 枚举 + 强类型 Message 结构体  
✅ **易于扩展**: 新增消息只需定义结构体和 toJson/fromJson  
✅ **调试友好**: JSON 可读，Header 固定格式易分析  

这种设计为多人在线卡牌游戏提供了稳定、灵活的通信基础。
