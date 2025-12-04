# 房间系统设计（创建房间 / 加入房间）

> 当前版本采用 **“每个服务器实例只承载单个房间”** 的 Host-Client 模式。
>
> - 客户端点击“创建房间”时，会在本机启动一个服务器实例，并作为第一个玩家加入该服务器；
> - 客户端点击“加入房间”时，只作为纯客户端连接到已有服务器（本地或远程）；
> - 将来可以扩展为“一个服务器进程管理多个房间”，但本设计以“单房间服务器”为主。

## 1. 总体目标

在现有“单场游戏、所有连接的客户端共享一局”的基础上，引入**房主 / 加入房主房间**机制，实现：

- 一个服务器进程 = 一个房间（Room）
- 每个客户端在任意时刻**只属于一个房间**
- 每个客户端最多在本机启动一个服务器实例，作为该房间房主
- 支持两种入口：
  - **创建房间**：在本机启动服务器，当前客户端作为房主加入
  - **加入房间**：当前客户端连接到远程（或本地）已有服务器
- 保持现有心跳、会话管理、ECS 游戏流程的整体架构不变

## 2. 概念与约束

### 2.1 概念

- **房间（Room） / 服务器实例**：
  - 一个服务器进程或嵌入式服务器对象就代表一个房间；
  - 维护一组玩家实体 `players: vector<entt::entity>`；
  - 记录房主 `owner`（一个玩家实体，通常是第一个登录的玩家）；
  - 持有该局游戏相关的 ECS 状态（仍然使用当前 `GameContext`，不做多房间拆分）。

- **房主（Room Owner）**：
  - 创建房间的客户端对应的玩家
  - 负责发起“开始游戏”操作
  - 可以对房间参数进行修改（人数上限、模式等，后期扩展）

- **大厅（Lobby）**：
  - 抽象概念，不必对应单独实体
  - 处于“未加入任何房间但已连接服务器”的客户端，可认为在大厅

### 2.2 约束

结合 `GameClient.h` 中的注释：

- “一个客户端是房间所有者，其他客户端是普通玩家”
- “一个客户端对应一个玩家，只能在一个房间内”
- “每一个 ip 只能有一个房间”

因此约束为：

1. **单房间服务器**：每个服务器实例只承载一个房间；
2. **每客户端最多一个本地服务器**：一个客户端进程只能在本机拥有一个服务器实例；
3. **一个客户端对应一个玩家，只能在一个房间内**：连接到某个服务器后，只参与该服务器的那一局游戏；
4. **断线重连保持房主/房间归属关系**：通过现有 `ClientSessionManager` 的 reconnect 能力恢复玩家实体与会话。

## 3. 协议扩展（消息类型）

在 **单房间服务器** 模式下，服务器已有的“登录 / 心跳 / 游戏状态 / 使用卡牌”等协议仍然适用。为了在 UI 上展示“房间信息”和便于后续扩展，可新增少量与房主身份相关的消息，但不再设计复杂的多房间 `RoomManager` 协议。

在 `src/shared/common/Common.h` 的 `enum class MessageType` 和协议文档 `docs/PacketHeader_And_Message_Protocol.md` 中新增/约定以下消息：

### 3.1 客户端 → 服务器

- `CREATE_ROOM`：请求创建房间
  - 载荷（JSON）：

    ```json
    {
      "roomName": "string",
      "maxPlayers": 4
    }
    ```

- `JOIN_ROOM`：请求加入某个房间
  - 载荷（JSON）：

    ```json
    {
      "roomId": 1001
    }
    ```

后续可以扩展：`LEAVE_ROOM`、`KICK_PLAYER`、`ROOM_SETTINGS_UPDATE` 等。

### 3.2 服务器 → 客户端

- `ROOM_CREATED`：房间创建成功
  - 载荷（JSON）：

    ```json
    {
      "roomId": 1001,
      "ownerEntity": 123,
      "maxPlayers": 4
    }
    ```

- `ROOM_CREATE_FAILED`：房间创建失败
  - 载荷（JSON）：

    ```json
    {
      "reason": "IP already has a room"
    }
    ```

- `ROOM_JOINED`：加入房间成功
  - 载荷（JSON）：

    ```json
    {
      "roomId": 1001,
      "playerEntity": 456,
      "players": [
        { "entity": 123, "name": "Owner" },
        { "entity": 456, "name": "Guest" }
      ]
    }
    ```

- `ROOM_JOIN_FAILED`：加入失败（房间不存在 / 已满等）
  - 载荷（JSON）：

    ```json
    {
      "reason": "room not found"
    }
    ```

- `ROOM_STATE_UPDATE`：房间玩家列表或配置更新
  - 载荷（JSON）：

    ```json
    {
      "roomId": 1001,
      "players": [ ... ],
      "maxPlayers": 4,
      "status": "waiting" | "playing"
    }
    ```

> 说明：
>
> - 在“一个服务器 = 一个房间”的模式下，`roomId` 可以直接固定为 `1` 或省略，仅用于 UI 显示；
> - 服务器内部只需要维护“当前所有在线玩家 + 谁是房主”即可。

- `GAME_START`：房间内游戏开始（可以复用 / 扩展现有 `GameStart` 事件）

## 4. 服务器端设计

### 4.1 单房间服务器中的房间信息

在当前单房间模式下，不再需要通用的多房间 `RoomManager`，而是直接在服务器（例如 `GameServer_Example` 或正式 `GameServer` 类）中维护：

```cpp
struct Room
{
  entt::entity owner{entt::null};
  std::vector<entt::entity> players;
  uint32_t maxPlayers{4};
  bool inGame{false};
};

class GameServer // 示例
{
public:
  // ... 现有接口

private:
  Room m_room; // 当前服务器唯一房间
};
```

与现有 `ClientSessionManager` 的协作：

- `ClientSessionManager` 负责 endpoint ↔ playerEntity ↔ clientId；
- `Room` 只关心“当前有哪些玩家、谁是房主”；
- 发送消息时：
  - 遍历 `m_room.players`；
  - 用 `ClientSessionManager::getEndpoint(entity)` 拿到 endpoint；
  - 然后调用 `NetWorkManager::sendPacket()`/`sendReliablePacket()`。

### 4.2 ECS 结构与房间

现有 `GameFlowSystem` 等系统默认操作“当前对局的所有玩家”。在 **单房间服务器** 模式下，可以保持：

- 使用一个全局 `GameContext` / `registry`；
- 所有玩家实体都属于当前唯一房间；
- 如需在未来支持多房间，再考虑引入 `RoomTag{ roomId }` 或为每个房间拆分 `GameContext`。

### 4.3 登录与房间流程

在 `GameServer_Example` 或正式服务器逻辑中扩展：

1. **登录阶段**（已存在）：
   - `handleLoginRequest` 创建玩家实体、注册客户端会话
   - 此时玩家还未进入房间，处于大厅状态

2. **创建房间（由“本地启动服务器”负责）**：

   - “创建房间”不需要单独的网络消息，而是通过在本机启动服务器实例来实现；
   - 第一个登录到该服务器的玩家自动成为房主：

   ```cpp
   void handleLoginRequest(const uint8_t* data, size_t size, const asio::ip::udp::endpoint& sender)
   {
     auto playerName = parsePlayerName(data, size);

     // 创建玩家实体 & 注册会话
     auto playerEntity = createPlayerEntity(playerName);
     uint32_t clientId = m_sessionManager->registerClient(sender, playerEntity, playerName);

     // 如果这是第一个玩家，则设为房主
     if (m_room.players.empty())
     {
       m_room.owner = playerEntity;
     }

     m_room.players.push_back(playerEntity);

     sendLoginSuccess(sender, clientId, playerEntity);
     broadcastPlayerJoined(playerName);
   }
   ```

3. **加入房间**：

   - 对服务器而言，“加入房间”就是普通登录请求；
   - 区别只是该服务器是由房主客户端本地启动还是独立进程启动，对协议无影响；
   - 可以通过 `m_room.players.size()` / `m_room.maxPlayers` 控制人数上限，超过则返回登录失败原因。

4. **开始游戏**：

   - 限制：只有房主可发 `GAME_START` 请求（消息可放在房间协议或复用现有 GameStart 事件）
   - 服务器在校验通过后：
     - 在该房间内创建/初始化所有与一局游戏相关的组件
     - 触发 `events::GameStart`，只针对该房间内玩家实体
     - 向房间内所有 endpoint 发送 `GAME_START`/`GAME_STATE` 消息

5. **断线重连与离开房间**：

   - 断线：
     - 保持 `RoomManager` 中的玩家列表不变，只设置 `ClientSession` 的 `isDisconnected = true`（已在 `ClientSessionManager` 中实现）
     - 房间内其他玩家可以收到“玩家掉线”广播
   - 重连：
     - 通过 `ClientSessionManager::reconnectClient` 恢复 endpoint
     - `RoomManager` 中原有的房间关系继续有效
   - 主动离开房间：
     - 新增 `LEAVE_ROOM` 消息和 `handleLeaveRoom`，从 `RoomManager` 移除玩家
     - 如果房主离开，策略可以是：
       - 解散房间
       - 或者把房主转移给下一位玩家

## 5. 客户端设计（含内嵌服务器 Host）

### 5.1 内嵌服务器启动接口草案

客户端在“创建房间”时，需要在本机启动一个服务器实例。这里给出两种方式：

#### 方式 A：同进程嵌入式服务器

在 `src/client/controller/` 或 `src/client/net/` 中新增一个简单的 Host 管理器：

```cpp
class LocalServerHost
{
public:
  static LocalServerHost& instance();

  bool start(uint16_t port);
  void stop();

  bool isRunning() const { return m_running; }

private:
  LocalServerHost() = default;

  std::atomic<bool> m_running{false};
  std::thread m_serverThread;
};
```

`start` 示例实现思路：

```cpp
bool LocalServerHost::start(uint16_t port)
{
  if (m_running.load())
  {
    return false; // 已有本地服务器
  }

  m_running.store(true);
  m_serverThread = std::thread([this, port]() {
    try
    {
      // 这里可以直接用 GameServer_Example 或正式 GameServer
      auto server = std::make_shared<GameServerExample>(port);
      server->run(); // 阻塞式运行在该线程
    }
    catch (...)
    {
      m_running.store(false);
    }
  });

  m_serverThread.detach();
  return true;
}
```

#### 方式 B：启动外部服务器进程

在 Windows 下可以通过 `CreateProcess` 或在当前项目中通过 PowerShell 调用外部可执行：

```cpp
bool LocalServerHost::start(uint16_t port)
{
  if (m_running.load())
  {
    return false;
  }

  // 伪代码：构造命令行，启动 .exe
  std::string cmd = "PestManKillServer.exe --port " + std::to_string(port);

  // 调用平台相关 API 启动进程，这里不展开具体实现

  m_running.store(true);
  return true;
}
```

两种方式都可以统一通过 `LocalServerHost` 暴露给 UI：

```cpp
bool ensureLocalServerRunning(uint16_t port)
{
  auto& host = LocalServerHost::instance();
  if (!host.isRunning())
  {
    return host.start(port);
  }
  return true;
}
```

### 5.2 `GameClient` 与房主判定

在 `GameClient` 中增加当前玩家是否为房主的简单判定：

```cpp
class GameClient : public std::enable_shared_from_this<GameClient>
{
public:
  // ... 现有接口

  bool isOwner() const { return m_isOwner; }

private:
  bool m_isOwner{false};
};
```

服务器端在第一个玩家登录时可以在登录成功返回中附带一个 `isOwner` 字段，客户端解析后：

```cpp
void handleLoginResponse(const uint8_t* data, size_t size)
{
  nlohmann::json json = nlohmann::json::parse(std::string{reinterpret_cast<const char*>(data), size});

  m_clientId = json.at("clientId").get<uint32_t>();
  m_playerEntity = json.at("entityId").get<uint32_t>();
  m_isOwner = json.value("isOwner", false);

  // ... 现有逻辑
}
```

这样 UI 就可以通过 `gameClient->isOwner()` 判断当前客户端是否是房主，从而决定是否展示【开始游戏】按钮等。

### 5.3 UI 流程（MainMenu / NetRoomWidget）

结合 `MainMenu.h` 注释“创建房间 / 加入房间 / 退出游戏”，推荐交互流程：

1. `MainMenu`：

   - 按钮【创建房间】：
     - 选择本地端口（例如固定 8888）和玩家昵称；
     - 调用 `ensureLocalServerRunning(port)` 启动本地服务器；
     - 启动成功后：

       ```cpp
       m_gameClient = std::make_shared<GameClient>();
       m_gameClient->setMessageHandler(handler);
       m_gameClient->connect("127.0.0.1", port);

       asio::co_spawn(
           utils::ThreadPool::getInstance().get_executor(),
           [client = m_gameClient, playerName]() -> asio::awaitable<void>
           {
               co_await client->login(playerName);
           },
           asio::detached);
       ```

   - 按钮【加入房间】：
     - 输入房主 IP、端口、玩家昵称；
     - 不启动本地服务器，只创建 `GameClient` 并连接：

       ```cpp
       m_gameClient = std::make_shared<GameClient>();
       m_gameClient->setMessageHandler(handler);
       m_gameClient->connect(serverIp, port);

       asio::co_spawn(
           utils::ThreadPool::getInstance().get_executor(),
           [client = m_gameClient, playerName]() -> asio::awaitable<void>
           {
               co_await client->login(playerName);
           },
           asio::detached);
       ```

2. `NetRoomWidget`：

   - 右侧显示房间信息（房主名称、人数/上限等，可由服务器通过 `ROOM_STATE_UPDATE` 或 `GAME_STATE` 携带）；
   - 左侧显示玩家列表（从 `GameClient` 的 `onGameStateUpdate` / 自定义房间状态回调中获取）；
   - 使用 `gameClient->isOwner()` 判定是否为房主：

     ```cpp
     if (m_gameClient->isOwner())
     {
         // 显示【开始游戏】按钮
     }
     else
     {
         // 只显示【准备】【退出】等
     }
     ```

3. `TestMessageHandler` 或正式 UI Handler 里：

   - 在 `onLoginSuccess` 或 `onGameStateUpdate` 中，根据服务器返回的数据更新房间界面；
   - 在 `onBroadcastEvent` 收到“玩家加入/离开/掉线”等文字事件时，刷新玩家列表；
   - 在未来若扩展 `ROOM_STATE_UPDATE` 消息时，可以专门解析房间信息刷新 UI。

## 6. 心跳与房间的关系

现有心跳机制是“客户端连接服务器后，周期性发送心跳包，服务器更新会话心跳时间”。

引入房间后：

- **不需要为每个房间单独做心跳**，仍然以客户端会话维度即可
- 但在心跳超时检测（`ClientSessionManager::checkTimeouts`）时：
  - 如果某个 session 断线，需要：
    - 在房间内标记对应玩家为掉线
    - 通过 `RoomManager` 广播“玩家掉线”事件给该房间其他玩家

这部分可以通过在 `checkTimeouts` 之后遍历超时实体，调用 `RoomManager` 做相应处理。

## 7. 迭代计划建议

为了避免一次性改动过大，建议按以下阶段落地：

1. **阶段 1：本地服务器启动 + 登录链路打通**

- 实现 `LocalServerHost`，支持在本机启动/停止单房间服务器；
- 在 `MainMenu` 上实现【创建房间】使用本地服务器、【加入房间】连接远程服务器；
- 保持现有 `GameServer_Example` 登录/心跳/广播逻辑不变。

2. **阶段 2：房主判定与 UI 集成**

- 服务器在登录成功响应中返回 `isOwner` 字段；
- `GameClient` 存储并暴露 `isOwner()` 接口；
- `NetRoomWidget` 根据 `isOwner()` 控制【开始游戏】按钮的显示。

3. **阶段 3：与 ECS GameFlow 打通**

- 房主点击【开始游戏】→ 发送 `GAME_START` 请求消息；
- 服务器在验证“请求者是房主”后，触发现有 `events::GameStart` 流程，对当前所有玩家生效。

4. **阶段 4：完善边界情况**

- 房主关闭客户端或本地服务器异常退出时，向所有玩家广播游戏结束/房间解散；
- 普通玩家掉线/退出时更新玩家列表并广播事件；
- 后续如需多房间或专用集中服务器，再在此基础上抽象出通用 `RoomManager`。

以上就是基于当前架构（`ClientSessionManager` + ECS + UDP + heartBeat）的“创建房间 / 加入房间”全局实现方案，可作为后续具体编码和重构的设计文档。
