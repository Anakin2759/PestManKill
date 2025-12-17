# 开发目的

持续改进与完善 UI 渲染框架、事件流和网络协议设计，目标是：

- 将渲染逻辑与 UI 状态分离（数据驱动），便于单元测试与替换渲染后端。
- 明确定义客户端内事件流与客户端-服务器交互流程，确保可恢复的可靠消息和重连策略。
- 提供可测试、可模拟的接口，方便用 GoogleTest 编写单元测试。

## 网络请求—响应总览

总体思想：客户端行为（例如“使用卡牌”）首先在本地触发事件并发起网络请求；服务器是权威，负责校验并广播结果。客户端对广播做幂等处理。

### 客户端流程（示例：使用卡牌）

1. 用户在 UI 上点击按钮，产生本地事件 `events::UseCardRequest`（包含 cardId、source、target 等）。
2. 客户端本地可以先做乐观更新（可选），同时封装消息并通过网络层发送 `UseCardMessage` 到服务器（可靠发送，带 seq）。
3. 服务器接收后验证合法性，修改服务器端游戏态并发送 `UseCardResponse`（广播到所有客户端）。
4. 客户端收到广播后：
   - 校验响应 seq / ack，更新本地游戏态（或回滚乐观更新）；
   - 派发本地事件（例如 `events::CardUsed`、`events::DamageApplied` 等）驱动 UI/动画/声音。

### 服务器流程

1. 接收并解析消息 -> 验证（玩家权限、回合、资源等）
2. 应用规则修改游戏状态（使用事务或事件队列）
3. 产生对所有客户端的广播消息（最终结果）；如果需要可靠传输则带上 seq/ack 与重试机制
4. 记录日志并将结果入持久化（可选）

## 网络通信协议（框架）

传输层：UDP（自定义可靠层）或在低延迟要求不高时考虑 TCP/DTLS。下面给出一个兼容可靠机制的简单包结构与 JSON payload 规范。

### PacketHeader

```cpp
struct PacketHeader {
        uint32_t seq;     // 序列号（发送者侧递增）
        uint32_t ack;     // 最新接收确认号（对端由此知道收到哪些包）
        uint16_t type;    // 消息类型（枚举）
        uint16_t flags;   // 标志位（可靠、心跳、压缩等）
        uint32_t size;    // payload 字节大小
};
```

说明：

- `flags` 可以包含是否需要 ACK、是否为分片等信息；
- `seq/ack` 用于实现 ACK+重传和简单丢包恢复；
- payload 推荐使用 JSON（可读）或二进制 protobuf（高效），当前项目用 JSON 以方便开发和调试。

### 示例消息（JSON payload）

UseCardMessage:

```json
{
    "type": "UseCard",
    "playerId": 3,
    "cardId": "slash_01",
    "targets": [1]
}
```

ServerBroadcast (UseCardResult):

```json
{
    "type": "UseCardResult",
    "seq": 1024,
    "actor": 3,
    "cardId": "slash_01",
    "effects": [ { "target":1, "hpChange": -1 } ],
    "state": { /* optional full-state or delta */ }
}
```

消息处理策略：广播消息应设计为幂等（可重复应用）；客户端收到旧的 seq 可以直接忽略或根据逻辑回滚。

## 心跳与重连

- 客户端需定期发送心跳 `HeartBeat` 包，服务器维护最后活动时间戳；超过超时阈值（例如 30s）标记为离线。
- 重连时客户端应携带最后已接收的 seq，以便服务器决定是否需要重发数据或发送同步快照。

## UI 框架与渲染设计

核心设计原则：组件只包含数据（属性），不包含绘制代码；所有绘制由 `RenderSystem`（或 `UIRenderSystem`）统一完成。渲染分派采用 variant/visitor 模式以便扩展。

### 组件（示例）

- `Position { x, y }`
- `Size { width, height }`
- `Visibility { visible, alpha }`
- `Widget { type }`  // type 可为 BUTTON, LABEL, IMAGE, LAYOUT ...
- `Clickable { text, enabled, uniqueId }`
- `ShowText { text, wrapWidth, textColor }`
- `Layout { direction, items, spacing, margins }` 等

所有 UI 元素都是 `entt::entity`，由工厂函数创建并返回实体句柄。

### 渲染分派（推荐：std::variant + std::visit）

渲染器维护一个从 widgetType -> visitor 的映射。渲染时：

1. 读取 `Widget` 类型，查表得到 `variant`（例如 `std::variant<std::monostate, ButtonDesc, LabelDesc, ...>` 或直接映射到可调用对象）；
2. 使用 `std::visit`（或调用 `std::function`）执行对应的渲染逻辑（统一接口，传入 entity + pos + size）；
3. 渲染子节点时先读取 `nextSibling`，再调用子渲染，避免在渲染过程中因为实体修改导致链表遍历错误。

优点：

- 易于扩展新的 widget 类型；
- 编译期能检查 variant 类型；
- 更接近数据驱动渲染思想，便于序列化/复用。

### 动画与过渡

将动画抽象为 `AnimationComponent`，由 `AnimationSystem` 驱动（基于时间步长更新属性，如 alpha、position、scale），并在 `RenderSystem` 读取最新属性进行绘制。动画支持 easing 函数、非匀速轨迹与回调。

## 单元测试策略（使用 GoogleTest）

目标：保证核心逻辑（事件分发、网络消息封装/解析、layout 算法、registry 操作等）可自动化测试，而不依赖实际渲染或窗口。

建议：

- 使用 `gtest`（FetchContent 拉取）并在 `tests/` 目录添加测试目标；
- 把可测试逻辑从系统中抽离为纯函数或小型类（例如 `LayoutCalculator`, `PacketSerializer`），这些模块没有 SDL/ImGui 依赖，易于测试；
- 对 `utils::Registry` 的操作可以在测试中创建独立实例或提供一个测试用的 Registry helper，确保测试间状态隔离；
- 对网络层使用接口抽象（例如 `INetworkTransport`），在测试中注入 Mock/Stub 来模拟丢包、延迟与重传场景；
- UI 渲染函数可以通过传入一个 `IDrawContext`（抽象）来替代 ImGui 调用，测试时提供一个记录调用的假对象。

示例测试场景：

- Layout 计算：给定一组 LayoutItem，断言每个子 widget 的最终 `Position/Size` 符合预期；
- Packet 序列化：序列化一个消息并反序列化，断言字段一致；
- Dispatcher：事件发送到监听者，断言回调被执行；
- 网络可靠层：注入假传输，模拟丢包并断言重试逻辑工作。

### CMake 与 gtest 集成示例（简短）

在项目根或 `third_party`：使用 `FetchContent` 拉取 googletest，然后创建测试可执行文件并链接项目内部模块（例如 `asio::asio`）。参考之前给出的 `tests/CMakeLists.txt` 示例。

## 可交付项与实现次序建议

短期（MVP）：

1. 抽离 `Layout` 计算到独立类并写单元测试；
2. 完成 `PacketSerializer`（JSON <-> message struct）并写序列化测试；
3. 把 `UIRenderSystem` 的分派改为 `std::variant + std::visit`，编写若干集成测试模拟顶层元素与子元素渲染顺序（不依赖真实 ImGui，实现一个 `IDrawRecorder`）；

中期：

1. 实现可靠 UDP 层的 ACK/重传与心跳机制并添加压力/丢包仿真测试；
2. 抽象网络接口，便于单元测试注入 Mock；

长期：

1. 增加端到端测试（集成服务器与两个客户端的最小场景），并使用 CI 运行这些测试（可在本地先用 Docker/虚拟网络模拟）；

## 约定与注意事项

- 所有实体/组件的 API 保持轻量，组件只存数据；系统负责行为与副作用；
- 在渲染/事件路径中注意并发与线程边界（网络线程、主线程、逻辑线程），使用事件队列或 dispatcher 做跨线程通信；
- 避免在渲染过程中直接修改层级链表（firstChild/nextSibling），如果必须修改请在帧结束应用变更或使用一套“变更队列”。
