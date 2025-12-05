/**
 * ************************************************************************

*
* @file ECS_UI_Usage_Example.md
* @author AnakinLiu (<azrael2759@qq.com>)
* @date 2025-12-05
* @version 0.1
* @brief ECS UI系统使用示例
*

 * ************************************************************************

* @copyright Copyright (c) 2025 AnakinLiu
* For study and research only, no reprinting.

 * ************************************************************************
 */

# ECS UI系统使用指南

## 概述

新的UI系统完全基于EnTT库的ECS（Entity-Component-System）架构重构，提供了更灵活、高性能的UI管理方式。

## 核心概念

### 1. 组件（Components）

所有UI数据都存储在组件中，位于 `src/client/components/UIComponents.h`：

* **基础组件**：`Position`, `Size`, `Visibility`, `Background`
* **布局组件**：`Layout`, `Spacer`
* **交互组件**：`Button`, `Label`, `TextEdit`, `Image`, `Arrow`
* **动画组件**：`Animation`, `PositionAnimation`, `AlphaAnimation`

### 2. 系统（Systems）

UI逻辑由系统处理：

* **UIRenderSystem** (`src/client/systems/UIRenderSystem.h`)：负责渲染所有UI元素
* **UIAnimationSystem** (`src/client/systems/UIAnimationSystem.h`)：负责更新动画

### 3. 工厂（Factory）

`UIFactory` (`src/client/utils/UIFactory.h`) 提供便捷的UI元素创建方法

## 基本使用

### 创建应用程序

```cpp
#include "src/client/ui/ECSApplication.h"

class MyApp : public ui::ECSApplication
{
public:
    MyApp() : ui::ECSApplication("My Application", 1280, 720)
    {
        setupUI();
    }

protected:
    void setupUI() override
    {
        auto& factory = getUiSystem().getFactory();
        auto& registry = getUiSystem().getRegistry();
        auto rootEntity = getRootEntity();

        // 创建按钮
        auto button = factory.createButton("Click Me", []() {
            utils::LOG_INFO("Button clicked!");
        });
        
        // 设置按钮属性
        ui::helper::setFixedSize(registry, button, 200.0F, 50.0F);
        ui::helper::setPosition(registry, button, 100.0F, 100.0F);

        // 将按钮添加到根布局
        factory.addWidgetToLayout(rootEntity, button);
    }
};

int main(int argc, char* argv[])
{
    try
    {
        MyApp app;
        app.run();
    }
    catch (const std::exception& e)
    {
        utils::LOG_ERROR("Application error: {}", e.what());
        return 1;
    }
    return 0;
}
```

### 创建布局

```cpp
auto& factory = getUiSystem().getFactory();
auto& registry = getUiSystem().getRegistry();

// 创建垂直布局
auto vLayout = factory.createVBoxLayout();
ui::helper::setLayoutMargins(registry, vLayout, 10.0F);
ui::helper::setLayoutSpacing(registry, vLayout, 5.0F);

// 创建水平布局
auto hLayout = factory.createHBoxLayout();

// 添加元素到布局
auto button1 = factory.createButton("Button 1");
auto button2 = factory.createButton("Button 2");

factory.addWidgetToLayout(hLayout, button1, 1); // stretch=1
factory.addStretchToLayout(hLayout, 2);          // 弹性空间
factory.addWidgetToLayout(hLayout, button2, 1);

// 嵌套布局
factory.addWidgetToLayout(vLayout, hLayout);
```

### 创建常用UI元素

#### 按钮

```cpp
auto button = factory.createButton("Submit", []() {
    // 点击回调
    utils::LOG_INFO("Submit clicked");
});

// 设置按钮属性
auto& buttonComp = registry.get<ui::components::Button>(button);
buttonComp.enabled = true;
buttonComp.tooltip = "Click to submit";
buttonComp.buttonColor = ImVec4(0.2F, 0.6F, 1.0F, 1.0F);
```

#### 文本标签

```cpp
auto label = factory.createLabel("Hello, ECS UI!");

auto& labelComp = registry.get<ui::components::Label>(label);
labelComp.textColor = ImVec4(1.0F, 1.0F, 1.0F, 1.0F);
labelComp.wordWrap = true;
```

#### 文本编辑框

```cpp
auto textEdit = factory.createTextEdit("Enter text...", false);

auto& textEditComp = registry.get<ui::components::TextEdit>(textEdit);
textEditComp.maxLength = 128;
textEditComp.onTextChanged = [](const std::string& newText) {
    utils::LOG_INFO("Text changed: {}", newText);
};
```

#### 图像

```cpp
auto image = factory.createImage(texturePtr);

auto& imageComp = registry.get<ui::components::Image>(image);
imageComp.maintainAspectRatio = true;
imageComp.tintColor = ImVec4(1.0F, 1.0F, 1.0F, 1.0F);
```

### 动画

#### 位置动画

```cpp
ImVec2 startPos{100.0F, 100.0F};
ImVec2 endPos{300.0F, 100.0F};
ui::helper::startPositionAnimation(registry, entity, startPos, endPos, 1.0F); // 1秒
```

#### 透明度动画

```cpp
ui::helper::startAlphaAnimation(registry, entity, 1.0F, 0.0F, 2.0F); // 淡出2秒
```

### 事件监听

```cpp
auto& dispatcher = getUiSystem().getDispatcher();

// 监听按钮点击事件
dispatcher.sink<ui::events::ButtonClicked>().connect<[](const ui::events::ButtonClicked& event) {
    utils::LOG_INFO("Button entity {} clicked", static_cast<uint32_t>(event.entity));
}>();

// 监听文本改变事件
dispatcher.sink<ui::events::TextChanged>().connect<[](const ui::events::TextChanged& event) {
    utils::LOG_INFO("Text changed to: {}", event.newText);
}>();
```

## 高级用法

### 自定义组件

```cpp
// 1. 在 UIComponents.h 中定义组件
struct MyCustomComponent
{
    std::string data;
    int value = 0;
};

// 2. 使用组件
auto entity = factory.createLabel("Custom");
registry.emplace<MyCustomComponent>(entity, "test", 42);

// 3. 在系统中处理
auto view = registry.view<MyCustomComponent, ui::components::Label>();
for (auto entity : view)
{
    auto& custom = view.get<MyCustomComponent>(entity);
    auto& label = view.get<ui::components::Label>(entity);
    // 处理逻辑
}
```

### 自定义系统

```cpp
class MyUISystem
{
public:
    MyUISystem(entt::registry& registry, entt::dispatcher& dispatcher)
        : m_registry(registry), m_dispatcher(dispatcher)
    {}

    void update(float deltaTime)
    {
        auto view = m_registry.view<MyCustomComponent>();
        for (auto entity : view)
        {
            auto& custom = view.get<MyCustomComponent>(entity);
            // 更新逻辑
        }
    }

private:
    entt::registry& m_registry;
    entt::dispatcher& m_dispatcher;
};

// 在 UiSystem 中添加
MyUISystem m_mySystem;
```

### 层次结构

```cpp
// 创建父子关系
auto parent = factory.createVBoxLayout();
auto child1 = factory.createButton("Child 1");
auto child2 = factory.createButton("Child 2");

factory.addChild(parent, child1);
factory.addChild(parent, child2);

// 访问层次结构
if (auto* hierarchy = registry.try_get<ui::components::Hierarchy>(parent))
{
    for (auto child : hierarchy->children)
    {
        // 处理子元素
    }
}
```

## 与旧系统的迁移

### 旧代码（OOP方式）

```cpp
auto button = std::make_shared<ui::Button>("Click Me");
button->setFixedSize(200, 50);
button->setPosition(100, 100);
button->setOnClick([]() { /* ... */ });
layout->addWidget(button);
```

### 新代码（ECS方式）

```cpp
auto button = factory.createButton("Click Me", []() { /* ... */ });
ui::helper::setFixedSize(registry, button, 200.0F, 50.0F);
ui::helper::setPosition(registry, button, 100.0F, 100.0F);
factory.addWidgetToLayout(layout, button);
```

## 性能优势

1. **缓存友好**：组件数据连续存储，提高缓存命中率
2. **批量处理**：系统可以批量处理相同类型的组件
3. **灵活组合**：通过组合组件而非继承实现功能
4. **事件驱动**：使用EnTT的dispatcher进行事件通信

## 调试技巧

```cpp
// 查看实体的所有组件
auto entity = /* ... */;
if (registry.all_of<ui::components::Position>(entity)) { /* ... */ }
if (registry.all_of<ui::components::Button>(entity)) { /* ... */ }

// 遍历所有按钮
auto view = registry.view<ui::components::Button>();
for (auto entity : view)
{
    const auto& button = view.get<ui::components::Button>(entity);
    utils::LOG_INFO("Button: {}", button.text);
}

// 销毁UI元素（会自动销毁子元素）
factory.destroyWidget(entity);
```

## 注意事项

1. **实体生命周期**：使用 `factory.destroyWidget()` 而非直接 `registry.destroy()`
2. **事件处理**：记得在适当时机调用 `dispatcher.update()` 来处理事件队列
3. **组件引用**：不要长期持有组件引用，每次使用时重新获取
4. **实体有效性**：使用 `registry.valid(entity)` 检查实体是否有效

## 完整示例

参考 `src/client/view/MainWindow.h` 查看完整的应用程序实现示例。
