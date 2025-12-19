# UI 模块使用指南

## 概述

UI 模块是基于 **EnTT ECS** 和 **ImGui** 的声明式 UI 框架，提供高性能、灵活的用户界面解决方案。

### 核心特性

- **ECS 架构**: 完全基于 Entity-Component-System 模式
- **声明式 API**: 通过工厂函数和辅助方法构建 UI
- **事件驱动**: 使用 EnTT Dispatcher 实现松耦合
- **响应式布局**: 自动布局计算和更新
- **动画支持**: 内置缓动函数和动画系统
- **ImGui 集成**: 底层使用 ImGui 绘制

---

## 架构设计

### 模块结构

```
src/ui/
├── components/          # ECS 组件定义
│   ├── UIComponents.h   # 核心组件（Position, Size, Text 等）
│   ├── UIDefine.h       # 枚举和常量定义
│   ├── UITags.h         # 标签组件（ButtonTag, WindowTag 等）
│   └── UIEvents.h       # 事件定义
├── systems/             # ECS 系统实现
│   ├── UIRenderSystem.h          # 渲染系统
│   ├── UILayoutSystem.h          # 布局系统
│   ├── UIAnimationSystem.h       # 动画系统
│   ├── InteractionSystem.h       # 交互系统
│   ├── BackgroundRenderSystem.h  # 背景渲染
│   └── WindowsSystem.h           # 窗口管理系统
└── ui/                  # 核心 API
    ├── UISystem.h       # 系统管理器
    ├── UIFactory.h      # UI 元素工厂
    ├── UIHelper.h       # 辅助工具函数
    └── Application.h    # 应用程序入口
```

### 系统更新顺序

```
Application.run()
    │
    └─> UISystem.update(deltaTime)
            │
            ├─> 1. InteractionSystem.update()      # 处理输入事件
            ├─> 2. UIAnimationSystem.update()      # 更新动画
            ├─> 3. UILayoutSystem.update()         # 计算布局
            ├─> 4. BackgroundRenderSystem.update() # 渲染背景
            ├─> 5. UIRenderSystem.update()         # 渲染UI元素
            └─> 6. WindowsSystem.update()          # 渲染窗口
```

---

## 核心概念

### 1. ECS 组件

#### 基础组件

```cpp
// 位置组件
struct Position {
    float x = 0.0f;
    float y = 0.0f;
};

// 尺寸组件
struct Size {
    float width = 0.0f;
    float height = 0.0f;
    float minWidth = 0.0f;
    float minHeight = 0.0f;
    float maxWidth = FLT_MAX;
    float maxHeight = FLT_MAX;
    bool autoSize = true;
};

// 文本组件
struct Text {
    std::string content;
    ImVec4 color = ImVec4(1, 1, 1, 1);
    float fontSize = 14.0f;
    Alignment alignment = Alignment::LEFT;
};

// 层次结构组件
struct Hierarchy {
    entt::entity parent = entt::null;
    std::vector<entt::entity> children;
};
```

#### 样式组件

```cpp
// 背景组件
struct Background {
    ImVec4 color = ImVec4(0, 0, 0, 0);
    float borderRadius = 0.0f;
    bool enabled = false;
};

// 边框组件
struct Border {
    ImVec4 color = ImVec4(1, 1, 1, 1);
    float thickness = 1.0f;
    bool enabled = false;
};

// 内边距组件
struct Padding {
    ImVec4 values{0, 0, 0, 0}; // Top, Right, Bottom, Left
};

// 外边距组件
struct Margin {
    ImVec4 values{0, 0, 0, 0};
};
```

#### 交互组件

```cpp
// 可点击组件
struct Clickable {
    std::function<void(entt::entity)> onClick;
    bool enabled = true;
};

// 按钮状态组件
struct ButtonState {
    bool isHovered = false;
    bool isPressed = false;
    bool isDisabled = false;
};

// 可拖拽组件
struct Draggable {
    bool isDragging = false;
    ImVec2 dragOffset{0, 0};
    bool constrainToParent = false;
};
```

#### 标签组件（Tag）

```cpp
// 按钮标签
struct ButtonTag {};

// 窗口标签
struct WindowTag {};

// 可见性标签
struct VisibleTag {};

// 布局脏标记
struct LayoutDirtyTag {};

// 悬停标签
struct HoveredTag {};

// 激活标签
struct ActiveTag {};
```

---

### 2. 事件系统

所有事件定义在 `UIEvents.h` 中：

```cpp
namespace ui::events {
    // 生命周期事件
    struct WidgetCreated { entt::entity entity; };
    struct WidgetDestroyed { entt::entity entity; };
    
    // 交互事件
    struct MouseEnter { entt::entity entity; float mouseX, mouseY; };
    struct MouseLeave { entt::entity entity; };
    struct Clicked { entt::entity entity; int button; };
    struct DragStart { entt::entity entity; float startX, startY; };
    
    // 值变更事件
    struct TextChanged { entt::entity entity; std::string oldText, newText; };
    struct SliderValueChanged { entt::entity entity; float oldValue, newValue; };
    
    // 布局事件
    struct SizeChanged { entt::entity entity; };
    struct PositionChanged { entt::entity entity; };
    
    // 窗口事件
    struct WindowOpened { entt::entity entity; std::string windowTitle; };
    struct WindowClosed { entt::entity entity; };
}
```

#### 事件订阅示例

```cpp
auto& dispatcher = utils::Dispatcher::getInstance();

// 订阅点击事件
dispatcher.sink<ui::events::Clicked>().connect<&onButtonClicked>();

void onButtonClicked(const ui::events::Clicked& event) {
    std::cout << "Button clicked: " << static_cast<uint32_t>(event.entity) << std::endl;
}
```

---

## 快速开始

### 1. 创建基础 UI

```cpp
#include "src/ui/ui/UIFactory.h"
#include "src/ui/ui/UIHelper.h"

using namespace ui;

auto& registry = utils::Registry::getInstance();

// 创建按钮
auto button = factory::CreateButton("Click Me");

// 设置样式
helper::setFixedSize(registry, button, 120, 40);
helper::setPosition(registry, button, 100, 100);
helper::setBackgroundColor(registry, button, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
helper::setBorderRadius(registry, button, 5.0f);

// 添加点击事件
auto& clickable = registry.get<components::Clickable>(button);
clickable.onClick = [](entt::entity entity) {
    std::cout << "Button clicked!" << std::endl;
};
```

---

### 2. 创建布局容器

```cpp
// 创建垂直布局容器
auto vbox = factory::CreateVBox();
helper::setFixedSize(registry, vbox, 300, 400);
helper::setPadding(registry, vbox, 10); // 10px 内边距

// 添加子元素
auto label = factory::CreateLabel("Username:");
auto input = factory::CreateTextInput("");
auto submitBtn = factory::CreateButton("Submit");

helper::addChild(registry, vbox, label);
helper::addChild(registry, vbox, input);
helper::addChild(registry, vbox, submitBtn);

// 设置间距
helper::setSpacing(registry, vbox, 8.0f);
```

---

### 3. 创建窗口

```cpp
// 创建模态窗口
auto window = factory::CreateWindow("Settings", 400, 300);

// 设置窗口属性
helper::setWindowResizable(registry, window, true);
helper::setWindowClosable(registry, window, true);
helper::centerWindow(registry, window);

// 添加内容
auto content = factory::CreateVBox();
helper::addChild(registry, window, content);

// 显示窗口
registry.emplace_or_replace<components::VisibleTag>(window);
```

---

### 4. 添加动画

```cpp
// 创建淡入动画
auto fadeInAnim = factory::CreateFadeInAnimation(button, 0.5f); // 0.5秒

// 创建位置动画
auto moveAnim = factory::CreateMoveAnimation(
    button,
    ImVec2(100, 100), // 起始位置
    ImVec2(300, 200), // 结束位置
    1.0f,             // 持续时间
    components::EasingType::EASE_OUT_CUBIC
);

// 监听动画完成事件
dispatcher.sink<ui::events::AnimationCompleted>().connect([](const auto& event) {
    std::cout << "Animation completed!" << std::endl;
});
```

---

## 完整示例

### 登录界面示例

```cpp
#include "src/ui/ui/Application.h"
#include "src/ui/ui/UIFactory.h"
#include "src/ui/ui/UIHelper.h"
#include "src/ui/ui/UISystem.h"

using namespace ui;

class LoginScene
{
public:
    void create() {
        auto& registry = utils::Registry::getInstance();

        // 创建主容器
        m_container = factory::CreateVBox();
        helper::setFixedSize(registry, m_container, 400, 300);
        helper::setPadding(registry, m_container, 20);
        helper::setBackgroundColor(registry, m_container, ImVec4(0.15f, 0.15f, 0.15f, 0.95f));
        helper::setBorderRadius(registry, m_container, 10.0f);
        helper::centerInParent(registry, m_container);

        // 标题
        auto title = factory::CreateLabel("Login");
        auto& titleText = registry.get<components::Text>(title);
        titleText.fontSize = 24.0f;
        titleText.alignment = components::Alignment::CENTER;
        helper::addChild(registry, m_container, title);

        // 用户名输入框
        auto usernameLabel = factory::CreateLabel("Username:");
        m_usernameInput = factory::CreateTextInput("");
        helper::setFixedSize(registry, m_usernameInput, 360, 30);
        helper::addChild(registry, m_container, usernameLabel);
        helper::addChild(registry, m_container, m_usernameInput);

        // 密码输入框
        auto passwordLabel = factory::CreateLabel("Password:");
        m_passwordInput = factory::CreateTextInput("");
        helper::setFixedSize(registry, m_passwordInput, 360, 30);
        helper::setPasswordMode(registry, m_passwordInput, true);
        helper::addChild(registry, m_container, passwordLabel);
        helper::addChild(registry, m_container, m_passwordInput);

        // 按钮容器（水平布局）
        auto buttonBox = factory::CreateHBox();
        helper::setSpacing(registry, buttonBox, 10);

        auto loginBtn = factory::CreateButton("Login");
        auto cancelBtn = factory::CreateButton("Cancel");

        helper::setFixedSize(registry, loginBtn, 175, 35);
        helper::setFixedSize(registry, cancelBtn, 175, 35);

        helper::setBackgroundColor(registry, loginBtn, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        helper::setBackgroundColor(registry, cancelBtn, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));

        // 添加点击事件
        auto& loginClickable = registry.get<components::Clickable>(loginBtn);
        loginClickable.onClick = [this](entt::entity) { onLoginClicked(); };

        auto& cancelClickable = registry.get<components::Clickable>(cancelBtn);
        cancelClickable.onClick = [this](entt::entity) { onCancelClicked(); };

        helper::addChild(registry, buttonBox, loginBtn);
        helper::addChild(registry, buttonBox, cancelBtn);
        helper::addChild(registry, m_container, buttonBox);

        // 淡入动画
        factory::CreateFadeInAnimation(m_container, 0.3f);
    }

    void destroy() {
        auto& registry = utils::Registry::getInstance();
        if (registry.valid(m_container)) {
            registry.destroy(m_container);
        }
    }

private:
    void onLoginClicked() {
        auto& registry = utils::Registry::getInstance();
        
        const auto& username = registry.get<components::TextInputState>(m_usernameInput).currentText;
        const auto& password = registry.get<components::TextInputState>(m_passwordInput).currentText;

        // 验证逻辑
        if (username.empty() || password.empty()) {
            showError("Please fill in all fields");
            return;
        }

        // 执行登录
        performLogin(username, password);
    }

    void onCancelClicked() {
        destroy();
    }

    void showError(const std::string& message) {
        // 创建错误提示窗口
        auto errorWindow = factory::CreateWindow("Error", 300, 150);
        auto errorLabel = factory::CreateLabel(message);
        helper::addChild(utils::Registry::getInstance(), errorWindow, errorLabel);
    }

    void performLogin(const std::string& username, const std::string& password) {
        // 实际登录逻辑
        std::cout << "Logging in: " << username << std::endl;
    }

    entt::entity m_container = entt::null;
    entt::entity m_usernameInput = entt::null;
    entt::entity m_passwordInput = entt::null;
};

// 主函数
int main() {
    Application app("PestManKill", 1280, 720);
    
    LoginScene loginScene;
    loginScene.create();

    app.run();

    loginScene.destroy();
    return 0;
}
```

---

## 最佳实践

### 1. 组件设计原则

✅ **DO**: 组件只包含数据，不包含逻辑

```cpp
struct Button {
    ImVec4 normalColor;
    ImVec4 hoverColor;
    ImVec4 pressedColor;
};
```

❌ **DON'T**: 在组件中添加方法

```cpp
struct Button {
    void onClick() { /* ... */ } // 错误！
};
```

---

### 2. 使用工厂函数创建元素

✅ **DO**: 使用工厂函数

```cpp
auto button = factory::CreateButton("Click Me");
```

❌ **DON'T**: 手动创建和配置组件

```cpp
auto entity = registry.create();
registry.emplace<Position>(entity);
registry.emplace<Size>(entity);
// ... 容易遗漏必要组件
```

---

### 3. 布局更新

在修改影响布局的属性后，始终标记为脏：

```cpp
helper::setSize(registry, entity, 100, 50); // 内部自动调用 markLayoutDirty
```

---

### 4. 事件处理

优先使用声明式事件订阅：

```cpp
// 订阅全局事件
dispatcher.sink<events::Clicked>().connect<&onAnyButtonClicked>();

// 或使用组件内回调
auto& clickable = registry.get<Clickable>(button);
clickable.onClick = [](entt::entity e) { /* ... */ };
```

---

### 5. 性能优化

- **批量操作**: 使用 `entt::registry::view` 批量处理实体
- **避免频繁创建/销毁**: 复用实体，使用 `VisibleTag` 控制显示
- **延迟布局更新**: 布局系统自动合并多次脏标记

---

## 常见问题

### Q: 为什么我的 UI 元素不显示？

**A**: 检查以下条件：

1. 是否有 `VisibleTag` 组件？
2. `Alpha` 组件的值是否大于 0？
3. `Size` 是否大于 0？
4. 父元素是否可见？

### Q: 如何实现自定义渲染？

**A**: 创建自定义系统并注册：

```cpp
class MyRenderSystem {
public:
    void update() {
        auto view = registry.view<MyComponent>();
        for (auto entity : view) {
            // 自定义渲染逻辑
        }
    }
};

// 在 UISystem 中添加
m_myRenderSystem.update();
```

### Q: 如何调试布局问题？

**A**: 使用辅助函数：

```cpp
helper::debugDrawBounds(drawList, pos, size); // 绘制边界框
helper::debugPrintHierarchy(registry, rootEntity); // 打印层次结构
```

---

## 参考资料

- [EnTT 文档](https://github.com/skypjack/entt)
- [ImGui 文档](https://github.com/ocornut/imgui)
- [ECS 架构模式](https://en.wikipedia.org/wiki/Entity_component_system)

---

## 作者

**AnakinLiu** (<azrael2759@qq.com>)  
版本: 0.1  
日期: 2025-12-18

---

## 许可证

仅供学习研究使用，禁止转载。  
Copyright (c) 2025 AnakinLiu
