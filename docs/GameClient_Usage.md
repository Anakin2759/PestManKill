# GameClient 使用指南

## 概览

`GameClient` 是客户端网络抽象层，封装了所有网络通信细节，提供高层游戏接口。

## 架构对比

### 服务器端架构

```
游戏逻辑 → GameServer → NetWorkManager → UDP
                ↓
        ClientSessionManager
```

### 客户端架构

```
游戏逻辑 → GameClient → NetWorkClient → UDP
            ↓
    IServerMessageHandler
```

## 核心组件

### 1. GameClient（网络管理器）

**职责**:

- 连接/断开服务器
- 登录/登出
- 发送游戏消息（使用卡牌、弃牌、结束回合等）
- 接收服务器消息并分发
- 自动心跳维护

### 2. IServerMessageHandler（消息处理接口）

**职责**:

- 定义服务器消息的回调接口
- 游戏层实现该接口来处理服务器事件

### 3. ClientState（连接状态）

```cpp
enum class ClientState {
    DISCONNECTED,    // 未连接
    CONNECTING,      // 连接中
    CONNECTED,       // 已连接（但未登录）
    AUTHENTICATED,   // 已认证（登录成功）
    IN_GAME          // 游戏中
};
```

## 基础用法

### 1. 创建客户端和消息处理器

```cpp
#include "src/client/net/GameClient.h"

// 实现消息处理器
class MyGameMessageHandler : public IServerMessageHandler {
public:
    void onConnected() override {
        std::cout << "已连接到服务器" << std::endl;
    }
    
    void onDisconnected() override {
        std::cout << "已断开连接" << std::endl;
    }
    
    void onLoginSuccess(uint32_t clientId, uint32_t playerEntity) override {
        std::cout << "登录成功！ClientID: " << clientId 
                  << ", Entity: " << playerEntity << std::endl;
    }
    
    void onLoginFailed(const std::string& reason) override {
        std::cout << "登录失败: " << reason << std::endl;
    }
    
    void onGameStart() override {
        std::cout << "游戏开始！" << std::endl;
    }
    
    void onGameEnd(const std::string& reason) override {
        std::cout << "游戏结束: " << reason << std::endl;
    }
    
    void onPlayerJoined(const std::string& playerName) override {
        std::cout << "玩家 " << playerName << " 加入了游戏" << std::endl;
    }
    
    void onPlayerLeft(const std::string& playerName) override {
        std::cout << "玩家 " << playerName << " 离开了游戏" << std::endl;
    }
    
    void onPlayerDisconnected(const std::string& playerName) override {
        std::cout << "玩家 " << playerName << " 掉线了" << std::endl;
    }
    
    void onPlayerReconnected(const std::string& playerName) override {
        std::cout << "玩家 " << playerName << " 重新连接" << std::endl;
    }
    
    void onGameStateUpdate(const nlohmann::json& gameState) override {
        std::cout << "游戏状态更新: " << gameState.dump() << std::endl;
    }
    
    void onUseCardResponse(bool success, const std::string& message) override {
        if (success) {
            std::cout << "使用卡牌成功: " << message << std::endl;
        } else {
            std::cout << "使用卡牌失败: " << message << std::endl;
        }
    }
    
    void onBroadcastEvent(const std::string& eventMessage) override {
        std::cout << "[广播] " << eventMessage << std::endl;
    }
    
    void onError(const std::string& errorMessage) override {
        std::cerr << "[错误] " << errorMessage << std::endl;
    }
};

// 创建客户端
auto gameClient = std::make_shared<GameClient>();
auto messageHandler = std::make_shared<MyGameMessageHandler>();

gameClient->setMessageHandler(messageHandler);
```

### 2. 连接服务器并登录

```cpp
// 连接服务器
gameClient->connect("127.0.0.1", 8888);

// 等待连接成功后登录
asio::co_spawn(
    utils::ThreadPool::getInstance().get_executor(),
    [gameClient]() -> asio::awaitable<void> {
        // 发送登录请求
        co_await gameClient->login("Player_A");
        
        // 登录成功后会触发 onLoginSuccess 回调
    },
    asio::detached
);
```

### 3. 发送游戏消息

#### 使用卡牌

```cpp
asio::co_spawn(
    utils::ThreadPool::getInstance().get_executor(),
    [gameClient]() -> asio::awaitable<void> {
        uint32_t cardEntity = 42;
        std::vector<uint32_t> targets = {7, 8}; // 目标玩家
        
        bool success = co_await gameClient->sendUseCard(cardEntity, targets);
        
        if (success) {
            std::cout << "发送使用卡牌消息成功" << std::endl;
        }
    },
    asio::detached
);
```

#### 弃牌

```cpp
asio::co_spawn(
    utils::ThreadPool::getInstance().get_executor(),
    [gameClient]() -> asio::awaitable<void> {
        std::vector<uint32_t> cards = {10, 11, 12};
        
        bool success = co_await gameClient->sendDiscardCard(cards);
        
        if (success) {
            std::cout << "发送弃牌消息成功" << std::endl;
        }
    },
    asio::detached
);
```

#### 结束回合

```cpp
asio::co_spawn(
    utils::ThreadPool::getInstance().get_executor(),
    [gameClient]() -> asio::awaitable<void> {
        bool success = co_await gameClient->sendEndTurn();
        
        if (success) {
            std::cout << "回合结束" << std::endl;
        }
    },
    asio::detached
);
```

#### 聊天消息

```cpp
asio::co_spawn(
    utils::ThreadPool::getInstance().get_executor(),
    [gameClient]() -> asio::awaitable<void> {
        co_await gameClient->sendChatMessage("大家好！");
    },
    asio::detached
);
```

### 4. 断开连接

```cpp
// 会自动发送登出消息并停止心跳
gameClient->disconnect();
```

## 与 UI 集成示例

### Qt/ImGui 集成

```cpp
class GameWindow : public ui::Application {
public:
    GameWindow() {
        // 创建客户端
        m_gameClient = std::make_shared<GameClient>();
        
        // 设置消息处理器
        auto handler = std::make_shared<GameWindowMessageHandler>(this);
        m_gameClient->setMessageHandler(handler);
    }
    
    void onConnectButtonClicked() {
        std::string serverIp = m_serverIpInput->getText();
        uint16_t port = std::stoi(m_portInput->getText());
        
        m_gameClient->connect(serverIp, port);
    }
    
    void onLoginButtonClicked() {
        std::string playerName = m_playerNameInput->getText();
        
        asio::co_spawn(
            utils::ThreadPool::getInstance().get_executor(),
            [self = this, playerName]() -> asio::awaitable<void> {
                co_await self->m_gameClient->login(playerName);
            },
            asio::detached
        );
    }
    
    void onUseCardButtonClicked(uint32_t cardId) {
        // 选择目标玩家
        auto targets = getSelectedTargets();
        
        asio::co_spawn(
            utils::ThreadPool::getInstance().get_executor(),
            [self = this, cardId, targets]() -> asio::awaitable<void> {
                bool success = co_await self->m_gameClient->sendUseCard(cardId, targets);
                
                if (!success) {
                    // 在 UI 线程显示错误
                    self->showError("发送失败");
                }
            },
            asio::detached
        );
    }
    
private:
    std::shared_ptr<GameClient> m_gameClient;
    
    // UI 组件
    std::shared_ptr<ui::TextEdit> m_serverIpInput;
    std::shared_ptr<ui::TextEdit> m_portInput;
    std::shared_ptr<ui::TextEdit> m_playerNameInput;
};

// 消息处理器实现
class GameWindowMessageHandler : public IServerMessageHandler {
public:
    GameWindowMessageHandler(GameWindow* window) : m_window(window) {}
    
    void onLoginSuccess(uint32_t clientId, uint32_t playerEntity) override {
        // 切换到游戏界面
        m_window->switchToGameScene();
    }
    
    void onGameStateUpdate(const nlohmann::json& gameState) override {
        // 更新 UI 显示
        m_window->updateGameState(gameState);
    }
    
    void onBroadcastEvent(const std::string& eventMessage) override {
        // 显示在聊天窗口
        m_window->addChatMessage(eventMessage);
    }
    
    // ... 其他回调
    
private:
    GameWindow* m_window;
};
```

## 状态管理

### 查询客户端状态

```cpp
ClientState state = gameClient->getState();

switch (state) {
    case ClientState::DISCONNECTED:
        std::cout << "未连接" << std::endl;
        break;
    case ClientState::CONNECTED:
        std::cout << "已连接，等待登录" << std::endl;
        break;
    case ClientState::AUTHENTICATED:
        std::cout << "已登录" << std::endl;
        break;
    case ClientState::IN_GAME:
        std::cout << "游戏中" << std::endl;
        break;
}
```

### 获取客户端信息

```cpp
uint32_t clientId = gameClient->getClientId();
uint32_t playerEntity = gameClient->getPlayerEntity();

std::cout << "ClientID: " << clientId << std::endl;
std::cout << "PlayerEntity: " << playerEntity << std::endl;
```

## 心跳机制

### 自动心跳

登录成功后会自动启动心跳（默认 5 秒间隔）：

```cpp
// 登录成功后自动启动
// gameClient->startHeartbeat(); // 内部自动调用
```

### 手动控制心跳

```cpp
// 自定义心跳间隔
gameClient->startHeartbeat(std::chrono::seconds(10));

// 停止心跳
gameClient->stopHeartbeat();
```

## 错误处理

### 网络错误

```cpp
class MyHandler : public IServerMessageHandler {
    void onError(const std::string& errorMessage) override {
        // 显示错误提示
        showErrorDialog(errorMessage);
        
        // 尝试重连
        if (errorMessage.find("连接断开") != std::string::npos) {
            attemptReconnect();
        }
    }
    
    void attemptReconnect() {
        // 延迟 3 秒后重连
        auto timer = std::make_shared<asio::steady_timer>(
            utils::ThreadPool::getInstance().get_executor(),
            std::chrono::seconds(3)
        );
        
        timer->async_wait([this](const asio::error_code& error) {
            if (!error) {
                m_gameClient->connect(m_lastServerIp, m_lastPort);
            }
        });
    }
};
```

### 超时处理

```cpp
// 发送超时会自动重试（最多 3 次）
bool success = co_await gameClient->sendUseCard(card, targets);

if (!success) {
    // 3 次重试后仍失败
    showError("网络不稳定，请检查连接");
}
```

## 完整示例：简单的游戏客户端

```cpp
#include "src/client/net/GameClient.h"
#include <iostream>
#include <thread>

class SimpleGameHandler : public IServerMessageHandler {
public:
    void onConnected() override {
        std::cout << "✓ 已连接到服务器" << std::endl;
    }
    
    void onLoginSuccess(uint32_t clientId, uint32_t playerEntity) override {
        std::cout << "✓ 登录成功！ID: " << clientId << std::endl;
        m_isLoggedIn = true;
    }
    
    void onLoginFailed(const std::string& reason) override {
        std::cout << "✗ 登录失败: " << reason << std::endl;
    }
    
    void onBroadcastEvent(const std::string& eventMessage) override {
        std::cout << "[服务器] " << eventMessage << std::endl;
    }
    
    void onGameStateUpdate(const nlohmann::json& gameState) override {
        std::cout << "游戏状态更新" << std::endl;
    }
    
    void onUseCardResponse(bool success, const std::string& message) override {
        if (success) {
            std::cout << "✓ " << message << std::endl;
        } else {
            std::cout << "✗ " << message << std::endl;
        }
    }
    
    // 实现其他必需的接口...
    void onDisconnected() override {}
    void onGameStart() override {}
    void onGameEnd(const std::string&) override {}
    void onPlayerJoined(const std::string&) override {}
    void onPlayerLeft(const std::string&) override {}
    void onPlayerDisconnected(const std::string&) override {}
    void onPlayerReconnected(const std::string&) override {}
    void onError(const std::string& error) override {
        std::cerr << "错误: " << error << std::endl;
    }
    
    bool isLoggedIn() const { return m_isLoggedIn; }
    
private:
    bool m_isLoggedIn = false;
};

int main() {
    // 创建客户端
    auto gameClient = std::make_shared<GameClient>();
    auto handler = std::make_shared<SimpleGameHandler>();
    gameClient->setMessageHandler(handler);
    
    // 连接服务器
    std::cout << "连接到服务器..." << std::endl;
    gameClient->connect("127.0.0.1", 8888);
    
    // 等待连接
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 登录
    asio::co_spawn(
        utils::ThreadPool::getInstance().get_executor(),
        [gameClient]() -> asio::awaitable<void> {
            std::cout << "发送登录请求..." << std::endl;
            co_await gameClient->login("Player_Test");
        },
        asio::detached
    );
    
    // 等待登录成功
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    if (handler->isLoggedIn()) {
        // 发送游戏消息
        asio::co_spawn(
            utils::ThreadPool::getInstance().get_executor(),
            [gameClient]() -> asio::awaitable<void> {
                // 使用卡牌
                std::cout << "使用卡牌..." << std::endl;
                co_await gameClient->sendUseCard(42, {7, 8});
                
                // 等待响应
                co_await asio::steady_timer(
                    utils::ThreadPool::getInstance().get_executor(),
                    std::chrono::seconds(1)
                ).async_wait(asio::use_awaitable);
                
                // 结束回合
                std::cout << "结束回合..." << std::endl;
                co_await gameClient->sendEndTurn();
            },
            asio::detached
        );
    }
    
    // 保持运行
    std::cout << "按回车键退出..." << std::endl;
    std::cin.get();
    
    // 断开连接
    gameClient->disconnect();
    
    return 0;
}
```

## 与服务器的消息对应关系

| 客户端发送 | 服务器接收 | 服务器响应 | 客户端接收 |
|-----------|-----------|-----------|-----------|
| `login("Player")` | `handleLogin()` | LOGIN_SUCCESS | `onLoginSuccess()` |
| `sendUseCard()` | `handleUseCard()` | USE_CARD_RESPONSE | `onUseCardResponse()` |
| `sendDiscardCard()` | `handleDiscardCard()` | - | - |
| `sendEndTurn()` | `handleEndTurn()` | - | - |
| `sendChatMessage()` | `handleChat()` | BROADCAST | `onBroadcastEvent()` |
| 自动心跳 | `updateHeartbeat()` | - | - |

## 注意事项

1. **协程使用**: 所有发送操作都返回 `asio::awaitable`，需要用 `asio::co_spawn` 调用
2. **线程安全**: `GameClient` 内部使用线程池，回调可能在非 UI 线程执行
3. **状态检查**: 发送游戏消息前检查 `getState()` 是否为 `IN_GAME`
4. **心跳维护**: 登录后自动启动，断开后自动停止
5. **消息格式**: 需要服务器返回带 `type` 字段的 JSON，或修改 `onPacketReceived` 逻辑

## 扩展建议

1. **消息队列**: 添加发送队列，批量发送消息
2. **重连机制**: 自动检测断线并尝试重连
3. **消息缓存**: 断线期间缓存消息，重连后重发
4. **协议版本**: 添加协议版本号验证
5. **加密传输**: 对敏感数据（如登录）进行加密
