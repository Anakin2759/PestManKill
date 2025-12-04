# MainMenu 使用示例

## 简介

`MainMenu` 是游戏的主菜单界面组件，提供创建房间、加入房间和退出游戏等功能。采用垂直列表布局，使用回调机制处理用户操作。

## 特性

- ✅ 垂直排列的按钮菜单
- ✅ 游戏标题显示
- ✅ 版本信息显示
- ✅ 深色半透明背景
- ✅ 回调机制处理按钮点击
- ✅ 可自定义标题文本

## 基本用法

### 1. 创建主菜单

```cpp
#include "src/client/view/MainMenu.h"
#include "src/client/utils/Logger.h"

// 定义回调函数
MainMenu::Callbacks callbacks;

callbacks.onCreateRoom = []() {
    utils::LOG_INFO("用户点击：创建房间");
    // 打开创建房间界面
    // showCreateRoomDialog();
};

callbacks.onJoinRoom = []() {
    utils::LOG_INFO("用户点击：加入房间");
    // 打开房间列表界面
    // showRoomListDialog();
};

callbacks.onExitGame = []() {
    utils::LOG_INFO("用户点击：退出游戏");
    // 退出应用
    // Application::quit();
};

// 创建主菜单
auto mainMenu = std::make_shared<MainMenu>(callbacks);
```

### 2. 在 Application 中使用

```cpp
#include "src/client/ui/Application.h"
#include "src/client/view/MainMenu.h"

class MyGameApp : public ui::Application {
public:
    MyGameApp(const std::string& title, int width, int height)
        : Application(title, width, height) {}

protected:
    void setupUI() override {
        // 创建主菜单
        MainMenu::Callbacks callbacks;
        callbacks.onCreateRoom = [this]() { onCreateRoom(); };
        callbacks.onJoinRoom = [this]() { onJoinRoom(); };
        callbacks.onExitGame = [this]() { quit(); };

        m_mainMenu = std::make_shared<MainMenu>(callbacks);
        
        // 设置为根布局
        auto layout = std::make_shared<ui::VBoxLayout>();
        layout->addWidget(m_mainMenu, 1);
        setRootLayout(layout);
    }

private:
    void onCreateRoom() {
        utils::LOG_INFO("创建房间...");
        // 切换到房间创建界面
        showCreateRoomView();
    }

    void onJoinRoom() {
        utils::LOG_INFO("加入房间...");
        // 切换到房间列表界面
        showRoomListView();
    }

    std::shared_ptr<MainMenu> m_mainMenu;
};
```

### 3. 完整的主程序示例

```cpp
#include "src/client/ui/Application.h"
#include "src/client/view/MainMenu.h"
#include "src/client/view/NetRoomWidget.h"
#include "src/client/net/GameClient.h"

enum class GameState {
    MAIN_MENU,
    ROOM_LIST,
    IN_ROOM,
    IN_GAME
};

class PestManKillApp : public ui::Application {
public:
    PestManKillApp() : Application("PestManKill", 1280, 720) {
        m_gameClient = std::make_shared<GameClient>();
    }

protected:
    void setupUI() override {
        m_rootLayout = std::make_shared<ui::VBoxLayout>();
        showMainMenu();
        setRootLayout(m_rootLayout);
    }

private:
    void showMainMenu() {
        m_currentState = GameState::MAIN_MENU;
        m_rootLayout->clear();

        MainMenu::Callbacks callbacks;
        callbacks.onCreateRoom = [this]() {
            utils::LOG_INFO("创建房间");
            createRoom();
        };
        callbacks.onJoinRoom = [this]() {
            utils::LOG_INFO("加入房间");
            showRoomList();
        };
        callbacks.onExitGame = [this]() {
            utils::LOG_INFO("退出游戏");
            quit();
        };

        auto mainMenu = std::make_shared<MainMenu>(callbacks);
        m_rootLayout->addWidget(mainMenu, 1);
    }

    void createRoom() {
        // 连接到服务器
        asio::co_spawn(
            utils::ThreadPool::getInstance().get_executor(),
            [this]() -> asio::awaitable<void> {
                co_await m_gameClient->connect("127.0.0.1", 8888);
                co_await m_gameClient->login("Player1");
                
                // 切换到房间界面
                showRoomView(true); // true 表示是房主
            },
            asio::detached);
    }

    void showRoomList() {
        m_currentState = GameState::ROOM_LIST;
        m_rootLayout->clear();

        // 显示房间列表界面
        // auto roomList = std::make_shared<RoomListWidget>();
        // m_rootLayout->addWidget(roomList, 1);
        
        utils::LOG_INFO("房间列表功能待实现");
        showMainMenu(); // 暂时返回主菜单
    }

    void showRoomView(bool isHost) {
        m_currentState = GameState::IN_ROOM;
        m_rootLayout->clear();

        // 显示房间界面
        // auto roomWidget = std::make_shared<NetRoomWidget>(m_gameClient, isHost);
        // m_rootLayout->addWidget(roomWidget, 1);
        
        utils::LOG_INFO("房间界面: 房主={}", isHost);
    }

    GameState m_currentState = GameState::MAIN_MENU;
    std::shared_ptr<GameClient> m_gameClient;
    std::shared_ptr<ui::VBoxLayout> m_rootLayout;
};

int main() {
    PestManKillApp app;
    return app.run();
}
```

### 4. 动态更新标题

```cpp
auto mainMenu = std::make_shared<MainMenu>(callbacks);

// 设置自定义标题
mainMenu->setTitle("害虫人杀 - 测试版");

// 或在运行时更新
mainMenu->setTitle("PestManKill v0.2");
```

### 5. 自定义样式

如果需要自定义菜单样式，可以继承 `MainMenu` 并重写 `setupUI()`:

```cpp
class CustomMainMenu : public MainMenu {
public:
    using MainMenu::MainMenu;

protected:
    void setupUI() override {
        // 调用基类实现
        MainMenu::setupUI();
        
        // 自定义背景颜色
        if (m_layout) {
            m_layout->setBackgroundColor(ImVec4(0.2F, 0.1F, 0.15F, 0.95F)); // 紫红色背景
        }
    }
};
```

## UI 结构

```
MainMenu (Widget)
└── VBoxLayout (主布局)
    ├── Stretch (1) - 顶部弹性空间
    ├── Label - 游戏标题 "PestManKill"
    ├── Spacer - 间距
    ├── ListArea (按钮区域)
    │   ├── Button - "创建房间" (300x50)
    │   ├── Button - "加入房间" (300x50)
    │   └── Button - "退出游戏" (300x50)
    ├── Stretch (1) - 底部弹性空间
    └── Label - 版本信息 "Version 0.1 - Learning Project"
```

## 尺寸和样式

### 默认尺寸

- 按钮宽度: 300px
- 按钮高度: 50px
- 按钮间距: 20px
- 边距: 40px (上下左右)

### 默认样式

- 背景颜色: `ImVec4(0.1F, 0.12F, 0.15F, 0.95F)` - 深色半透明
- 按钮间距: 20px
- 标题尺寸: 400x80

## 回调说明

### Callbacks 结构体

```cpp
struct Callbacks {
    std::function<void()> onCreateRoom;  // 点击"创建房间"按钮时触发
    std::function<void()> onJoinRoom;    // 点击"加入房间"按钮时触发
    std::function<void()> onExitGame;    // 点击"退出游戏"按钮时触发
};
```

所有回调都是可选的。如果未设置回调，点击按钮不会有任何效果。

## 完整的游戏流程示例

```cpp
class GameFlow {
public:
    void start() {
        showMainMenu();
    }

private:
    void showMainMenu() {
        MainMenu::Callbacks callbacks;
        
        callbacks.onCreateRoom = [this]() {
            // 1. 连接服务器
            connectToServer();
            
            // 2. 创建房间
            createNewRoom();
            
            // 3. 进入房间等待界面
            enterRoomLobby(true); // 作为房主
        };
        
        callbacks.onJoinRoom = [this]() {
            // 1. 连接服务器
            connectToServer();
            
            // 2. 获取房间列表
            fetchRoomList();
            
            // 3. 显示房间列表让玩家选择
            showRoomSelection();
        };
        
        callbacks.onExitGame = [this]() {
            // 清理资源
            cleanup();
            
            // 退出应用
            exit(0);
        };
        
        auto menu = std::make_shared<MainMenu>(callbacks);
        displayWidget(menu);
    }
    
    void connectToServer() {
        asio::co_spawn(
            utils::ThreadPool::getInstance().get_executor(),
            [this]() -> asio::awaitable<void> {
                co_await m_gameClient->connect("127.0.0.1", 8888);
                co_await m_gameClient->login(m_playerName);
                utils::LOG_INFO("成功连接到服务器");
            },
            asio::detached);
    }
    
    void createNewRoom() {
        // 发送创建房间请求
        // m_gameClient->sendCreateRoomRequest(...);
    }
    
    void enterRoomLobby(bool isHost) {
        // 切换到房间等待界面
        // auto roomWidget = std::make_shared<NetRoomWidget>(m_gameClient, isHost);
        // displayWidget(roomWidget);
    }
    
    void fetchRoomList() {
        // 请求房间列表
        // m_gameClient->sendGetRoomListRequest();
    }
    
    void showRoomSelection() {
        // 显示房间列表界面
        // auto roomList = std::make_shared<RoomListWidget>();
        // displayWidget(roomList);
    }
    
    void cleanup() {
        if (m_gameClient) {
            m_gameClient->disconnect();
        }
    }
    
    void displayWidget(std::shared_ptr<ui::Widget> widget) {
        // 实际显示逻辑
    }
    
    std::shared_ptr<GameClient> m_gameClient;
    std::string m_playerName = "DefaultPlayer";
};
```

## 注意事项

1. **回调执行**: 回调在 UI 线程执行，网络操作应使用 `asio::co_spawn` 异步执行
2. **内存管理**: MainMenu 使用 `shared_ptr` 管理子组件，自动释放资源
3. **样式定制**: 通过 `setBackgroundColor()` 等方法自定义外观
4. **响应式布局**: 使用 `Stretch` 实现垂直居中效果
5. **按钮尺寸**: 所有按钮固定为 300x50，保持一致性

## 常见问题

**Q: 如何添加更多按钮？**

A: 修改 `createButtonArea()` 方法，添加新按钮到 `buttonList`。

**Q: 如何改变按钮顺序？**

A: 在 `createButtonArea()` 中调整 `addWidget()` 的调用顺序。

**Q: 点击按钮没有反应？**

A: 检查是否设置了对应的回调函数。

**Q: 如何隐藏某个按钮？**

A: 可以在创建按钮后调用 `button->setVisible(false)`，或者修改 `createButtonArea()` 不添加该按钮。

## 参考

- `src/client/ui/Widget.h` - 基础组件
- `src/client/ui/Button.h` - 按钮组件
- `src/client/ui/ListArea.h` - 列表区域组件
- `src/client/ui/Layout.h` - 布局管理器
- `src/client/view/MainWindow.h` - 主窗口示例
