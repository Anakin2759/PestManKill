# Client 模块使用指南

## 概述

Client 模块是 **PestManKill** 的客户端实现，完整集成了 **UI 模块**和 **Net 模块**，提供图形化游戏客户端功能。

### 核心特性

- ✅ **网络通信**: 基于 KCP + ASIO 的可靠 UDP 连接
- ✅ **UI 系统**: ImGui + ECS 架构的声明式 UI
- ✅ **场景管理**: 灵活的场景切换系统
- ✅ **消息处理**: 自动消息序列化/反序列化
- ✅ **事件驱动**: 松耦合的事件系统

---

## 架构设计

### 模块依赖关系

```
┌─────────────────────────────────────────┐
│         Client Application              │
│  (main.cpp + SDL3 + ImGui Backend)     │
└──────────────┬──────────────────────────┘
               │
       ┌───────┴────────┐
       │   GameClient   │  ← 核心客户端类
       └───────┬────────┘
               │
   ┌───────────┼───────────┐
   │           │           │
   ▼           ▼           ▼
┌────────┐ ┌────────┐ ┌────────┐
│  Net   │ │   UI   │ │ Shared │
│ Module │ │ Module │ │Messages│
└────────┘ └────────┘ └────────┘
```

### 类结构

```
Application (main.cpp)
    │
    ├─> GameClient
    │       ├─> Client (Net Module)
    │       ├─> AsioUdpTransport
    │       ├─> KcpSession
    │       └─> MessageDispatcher
    │
    └─> SceneManager
            ├─> LoginScene
            ├─> MainMenuScene
            ├─> RoomScene
            └─> GameScene
```

---

## 核心类

### 1. GameClient

**位置**: `src/client/GameClient.h`

客户端核心类，负责网络连接、消息处理和 UI 更新。

```cpp
class GameClient {
public:
    GameClient(const std::string& serverAddr, 
               uint16_t serverPort, 
               const std::string& playerName);

    bool initialize();        // 初始化客户端
    void run();              // 主循环
    void stop();             // 停止客户端

    template <typename MessageType>
    void sendMessage(const MessageType& message);

    bool isConnected() const;
};
```

**主要职责**:

- 管理网络连接生命周期
- 处理服务器消息
- 协调 UI 系统更新
- 事件分发

---

### 2. SceneManager

**位置**: `src/client/SceneManager.h`

场景管理器，负责 UI 场景的切换和生命周期管理。

```cpp
class SceneManager {
public:
    static SceneManager& getInstance();

    template <typename SceneType>
    void registerScene(const std::string& name);

    void switchTo(const std::string& sceneName);
    void update(float deltaTime);
};
```

**内置场景**:

- `LoginScene` - 登录界面
- `MainMenuScene` - 主菜单
- `RoomScene` - 房间界面
- `GameScene` - 游戏界面

---

### 3. Scene 基类

**位置**: `src/client/SceneManager.h`

所有场景的基类。

```cpp
class Scene {
public:
    virtual void enter() = 0;    // 进入场景
    virtual void exit() = 0;     // 退出场景
    virtual void update(float deltaTime) = 0;

protected:
    GameClient* m_client;
    entt::registry& m_registry;
};
```

---

## 场景实现

### LoginScene（登录场景）

**功能**:

- 用户名输入
- 密码输入（隐藏显示）
- 登录按钮
- 退出按钮

**UI 结构**:

```
VBox (主容器)
  ├─ Label (标题: "PestManKill")
  ├─ Label (副标题: "Welcome Back!")
  ├─ Spacer (间隔)
  ├─ Label ("Username")
  ├─ TextInput (用户名输入框)
  ├─ Label ("Password")
  ├─ TextInput (密码输入框，密码模式)
  ├─ Spacer
  └─ HBox (按钮容器)
      ├─ Button (Login)
      └─ Button (Quit)
```

**使用示例**:

```cpp
void LoginScene::onLoginButtonClicked() {
    auto& username = m_registry.get<TextInputState>(m_usernameInput).currentText;
    auto& password = m_registry.get<TextInputState>(m_passwordInput).currentText;

    if (m_client->isConnected()) {
        SceneManager::getInstance().switchTo("MainMenu");
    }
}
```

---

### MainMenuScene（主菜单）

**功能**:

- 创建房间
- 加入房间
- 设置
- 登出

**UI 布局**:

```
VBox
  ├─ Label (标题)
  ├─ Label (玩家信息)
  ├─ Spacer
  ├─ Button (Create Room)
  ├─ Button (Join Room)
  ├─ Button (Settings)
  └─ Button (Logout)
```

**消息发送**:

```cpp
void MainMenuScene::onCreateRoomClicked() {
    auto req = CreateRoomRequest::create("My Room", 8, 0);
    m_client->sendMessage(req);
}
```

---

## 网络通信

### 消息发送流程

```cpp
// 1. 构造消息
auto req = CreateRoomRequest::create("TestRoom", 5, 0);

// 2. 发送消息
m_client->sendMessage(req);

// 内部流程:
// encodeMessage(req)
//   ↓ 序列化为二进制
// encodeFrame(buffer, CMD_ID, payload)
//   ↓ 添加帧头
// session->send(frameData)
//   ↓ KCP 发送
// UDP 传输
```

### 消息接收流程

```cpp
UDP 接收
  ↓
Client::input(from, data)
  ↓
KcpSession::input(data)
  ↓
KcpSession::recv() (协程)
  ↓
decodeFrame(kcpData)
  ↓
MessageDispatcher::dispatch(cmd, payload)
  ↓
注册的处理器回调
```

### 注册消息处理器

```cpp
m_dispatcher.registerHandler<CreateRoomResponse>([this](const CreateRoomResponse& resp) {
    if (resp.success) {
        m_logger->info("Room created: {}", resp.roomId);
        onRoomCreated(resp.roomId);
    }
    return std::vector<uint8_t>{}; // 无需响应
});
```

---

## 构建和运行

### 构建客户端

```powershell
# 配置项目
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# 构建客户端
cmake --build build --config Release --target PestManKillClient

# 运行
./build/bin/PestManKillClient.exe
```

### 依赖项

客户端依赖以下库：

- **SDL3**: 窗口和渲染
- **ImGui**: UI 渲染
- **ASIO**: 网络通信
- **EnTT**: ECS 框架
- **KCP**: 可靠 UDP 协议
- **spdlog**: 日志
- **nlohmann_json**: JSON 序列化

---

## 配置选项

### 服务器配置

在 `main.cpp` 中修改服务器地址：

```cpp
// 默认配置
m_gameClient = std::make_unique<GameClient>(
    "127.0.0.1",  // 服务器地址
    9999,         // 服务器端口
    "Player1"     // 玩家名称
);
```

### 窗口配置

```cpp
Application app(
    "PestManKill Client",  // 窗口标题
    1280,                  // 宽度
    720                    // 高度
);
```

### UI 样式配置

在 `Application::customizeImGuiStyle()` 中自定义：

```cpp
ImGuiStyle& style = ImGui::GetStyle();
style.WindowRounding = 8.0f;
style.FrameRounding = 6.0f;
// ...更多样式配置
```

---

## 添加新场景

### 步骤1: 定义场景类

```cpp
// MyCustomScene.h
class MyCustomScene : public Scene {
public:
    void enter() override;
    void exit() override;
    void update(float deltaTime) override;

private:
    void createUI();
    entt::entity m_rootEntity = entt::null;
};
```

### 步骤2: 实现场景逻辑

```cpp
// MyCustomScene.cpp
void MyCustomScene::enter() {
    createUI();
}

void MyCustomScene::createUI() {
    m_rootEntity = factory::CreateVBox();
    // ...创建 UI 元素
}

void MyCustomScene::exit() {
    if (m_registry.valid(m_rootEntity)) {
        m_registry.destroy(m_rootEntity);
    }
}

void MyCustomScene::update(float deltaTime) {
    // 场景特定逻辑
}
```

### 步骤3: 注册场景

```cpp
// SceneManager 构造函数中
registerScene<MyCustomScene>("MyCustom");

// 切换到新场景
SceneManager::getInstance().switchTo("MyCustom");
```

---

## 添加新消息类型

### 步骤1: 定义消息结构

```cpp
// src/shared/messages/request/MyRequest.h
struct MyRequest : public MessageBase<MyRequest> {
    static constexpr uint16_t CMD_ID = 0x4100;

    uint32_t data1;
    char text[64];

    std::expected<std::span<uint8_t>, MessageError> 
    serializeImpl(std::span<uint8_t> buffer) const;

    static std::expected<MyRequest, MessageError> 
    deserializeImpl(std::span<const uint8_t> data);
};
```

### 步骤2: 注册处理器

```cpp
// GameClient::setupMessageHandlers()
m_dispatcher.registerHandler<MyRequest>([this](const MyRequest& req) {
    // 处理逻辑
    m_logger->info("Received MyRequest: {}", req.data1);
    return std::vector<uint8_t>{};
});
```

### 步骤3: 发送消息

```cpp
MyRequest req;
req.data1 = 12345;
std::strncpy(req.text, "Hello", 63);

m_client->sendMessage(req);
```

---

## 调试技巧

### 1. 启用详细日志

```cpp
m_logger->set_level(spdlog::level::debug);
```

### 2. 查看网络数据包

```cpp
// 在 GameClient::sendMessage 中添加日志
m_logger->debug("Sending: cmd=0x{:04X}, size={}", 
                MessageType::CMD_ID, encoded->size());
```

### 3. 调试 UI 层次结构

```cpp
#include "src/ui/ui/UIHelper.h"

// 打印 UI 树
helper::debugPrintHierarchy(m_registry, rootEntity);

// 绘制边界框
helper::debugDrawBounds(drawList, pos, size);
```

### 4. 性能分析

```cpp
auto start = std::chrono::high_resolution_clock::now();
// ... 代码
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
m_logger->debug("Operation took: {} μs", duration.count());
```

---

## 常见问题

### Q: 客户端无法连接到服务器？

**A**: 检查：

1. 服务器是否启动
2. 地址和端口是否正确
3. 防火墙设置
4. 查看日志输出

### Q: UI 元素不显示？

**A**: 确保：

1. 实体有 `VisibleTag` 组件
2. `Size` 大于 0
3. `Alpha` 大于 0
4. 父元素可见

### Q: 消息发送失败？

**A**: 检查：

1. 是否已连接（`isConnected()`）
2. 消息序列化是否正确
3. 命令ID是否正确注册

---

## 性能优化

### 1. 网络优化

- 批量发送小消息
- 使用心跳保活
- 合理设置 KCP 参数

### 2. UI 优化

- 复用实体，避免频繁创建/销毁
- 使用 `LayoutDirtyTag` 避免不必要的布局计算
- 隐藏而非销毁不常用的 UI

### 3. 渲染优化

- 控制帧率（VSync）
- 减少 ImGui 绘制调用
- 使用 ImGui 的剪裁功能

---

## 参考资料

- [Net 模块文档](net_module_guide.md)
- [UI 模块文档](ui_module_guide.md)
- [Shared 模块文档](shared_module_guide.md)
- [ImGui 文档](https://github.com/ocornut/imgui)
- [SDL3 文档](https://wiki.libsdl.org/SDL3)

---

## 作者

**AnakinLiu** (<azrael2759@qq.com>)  
版本: 0.1  
日期: 2025-12-18

---

## 许可证

仅供学习研究使用，禁止转载。  
Copyright (c) 2025 AnakinLiu
