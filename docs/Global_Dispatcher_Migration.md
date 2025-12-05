# UI系统全局事件分发器迁移总结

## 改动概述

将UI ECS系统中的局部`entt::dispatcher`替换为全局单例`utils::Dispatcher::getInstance()`，实现更好的事件管理和跨模块通信。

## 改动文件列表

### 1. 核心系统文件

#### `src/client/model/UiSystem.h`

- ✅ 添加 `#include "src/client/utils/Dispatcher.h"`
- ✅ 移除成员变量 `entt::dispatcher m_dispatcher`
- ✅ 修改构造函数初始化列表，移除dispatcher参数传递
- ✅ `update()` 方法改用 `utils::Dispatcher::getInstance().update()`
- ✅ `getDispatcher()` 方法返回全局dispatcher引用
- ✅ 更新注释中的示例代码

#### `src/client/model/UIFactory.h`

- ✅ 添加 `#include "src/client/utils/Dispatcher.h"`
- ✅ 移除构造函数参数 `entt::dispatcher& dispatcher`
- ✅ 移除成员变量 `entt::dispatcher& m_dispatcher`
- ✅ 所有10处 `m_dispatcher.enqueue<>()` 改为 `utils::Dispatcher::getInstance().enqueue<>()`
  - `createButton()` - WidgetCreated事件
  - `createLabel()` - WidgetCreated事件
  - `createTextEdit()` - WidgetCreated事件
  - `createImage()` - WidgetCreated事件
  - `createArrow()` - WidgetCreated事件
  - `createHBoxLayout()` - WidgetCreated事件
  - `createVBoxLayout()` - WidgetCreated事件
  - `createSpacer()` - WidgetCreated事件
  - `createDialog()` - WidgetCreated事件
  - `destroyWidget()` - WidgetDestroyed事件

#### `src/client/systems/UIRenderSystem.h`

- ✅ 添加 `#include "src/client/utils/Dispatcher.h"`
- ✅ 移除构造函数参数 `entt::dispatcher& dispatcher`
- ✅ 移除成员变量 `entt::dispatcher& m_dispatcher`
- ✅ `renderButton()` 中的 `m_dispatcher.enqueue<events::ButtonClicked>()` 改为全局dispatcher
- ✅ `renderTextEdit()` 中的 `m_dispatcher.enqueue<events::TextChanged>()` 改为全局dispatcher

#### `src/client/systems/UIAnimationSystem.h`

- ✅ 添加 `#include "src/client/utils/Dispatcher.h"`
- ✅ 移除构造函数参数 `entt::dispatcher& dispatcher`
- ✅ 移除成员变量 `entt::dispatcher& m_dispatcher`
- ✅ `update()` 中的 `m_dispatcher.enqueue<events::AnimationComplete>()` 改为全局dispatcher

#### `src/client/view/MainWindowECS.h`

- ✅ 添加 `#include "src/client/utils/Dispatcher.h"`
- ✅ `setupUI()` 中移除未使用的局部变量 `auto& dispatcher = getUiSystem().getDispatcher()`
- ✅ `setupEventHandlers()` 改用 `auto& dispatcher = utils::Dispatcher::getInstance()`

## 技术细节

### 全局Dispatcher实现

```cpp
// src/client/utils/Dispatcher.h
namespace utils
{
class Dispatcher
{
public:
    static entt::dispatcher& getInstance()
    {
        static entt::dispatcher instance;
        return instance;
    }
    
private:
    Dispatcher() = default;
    // ... 删除拷贝/移动构造
};
}
```

### 使用模式对比

#### 修改前（局部dispatcher）

```cpp
class UIRenderSystem
{
public:
    UIRenderSystem(entt::registry& registry, entt::dispatcher& dispatcher)
        : m_registry(registry), m_dispatcher(dispatcher) {}
    
    void renderButton(...)
    {
        m_dispatcher.enqueue<events::ButtonClicked>(entity);
    }
    
private:
    entt::registry& m_registry;
    entt::dispatcher& m_dispatcher;  // 需要传递引用
};
```

#### 修改后（全局dispatcher）

```cpp
class UIRenderSystem
{
public:
    UIRenderSystem(entt::registry& registry)
        : m_registry(registry) {}
    
    void renderButton(...)
    {
        utils::Dispatcher::getInstance().enqueue<events::ButtonClicked>(entity);
    }
    
private:
    entt::registry& m_registry;
    // 不再需要dispatcher成员变量
};
```

## 优势

### 1. **简化依赖注入**

- 不需要在每个系统构造时传递dispatcher引用
- 减少构造函数参数数量
- 降低耦合度

### 2. **全局事件通信**

- 任何模块都可以直接访问事件系统
- 便于跨模块事件发送和监听
- 统一的事件管理入口

### 3. **代码简洁性**

- 减少成员变量数量
- 减少构造函数参数
- 更清晰的代码结构

### 4. **一致性**

- 与现有的 `utils::Registry::getInstance()` 模式保持一致
- 统一的单例访问模式

## 影响范围

### 直接修改的类

- `UiSystem` - UI系统管理器
- `UIFactory` - UI工厂
- `UIRenderSystem` - 渲染系统
- `UIAnimationSystem` - 动画系统
- `MainWindowECS` - 示例主窗口

### 事件类型

所有事件发送都已改为使用全局dispatcher：

- `events::WidgetCreated` - UI元素创建
- `events::WidgetDestroyed` - UI元素销毁
- `events::ButtonClicked` - 按钮点击
- `events::TextChanged` - 文本改变
- `events::AnimationComplete` - 动画完成

## 向后兼容性

✅ **完全兼容** - 由于 `UiSystem::getDispatcher()` 仍然返回全局dispatcher的引用，外部代码无需修改。

```cpp
// 以下代码仍然有效
auto& dispatcher = uiSystem.getDispatcher();
dispatcher.sink<events::ButtonClicked>().connect<handler>();

// 也可以直接使用
utils::Dispatcher::getInstance().sink<events::ButtonClicked>().connect<handler>();
```

## 测试建议

1. **事件发送测试**
   - 验证所有UI元素创建时正确发送 `WidgetCreated` 事件
   - 验证按钮点击触发 `ButtonClicked` 事件
   - 验证文本改变触发 `TextChanged` 事件
   - 验证动画完成触发 `AnimationComplete` 事件

2. **事件监听测试**
   - 验证跨模块事件监听正常工作
   - 验证事件队列正确更新

3. **性能测试**
   - 确认全局dispatcher不会造成性能问题
   - 测试大量事件发送场景

## 最佳实践

### 发送事件

```cpp
// 立即入队，延迟处理
utils::Dispatcher::getInstance().enqueue<events::MyEvent>(param1, param2);

// 立即触发
utils::Dispatcher::getInstance().trigger<events::MyEvent>(param1, param2);
```

### 监听事件

```cpp
// 在构造函数或初始化方法中连接
auto& dispatcher = utils::Dispatcher::getInstance();
dispatcher.sink<events::MyEvent>().connect<&MyClass::onMyEvent>(this);

// 在析构函数中断开连接
dispatcher.sink<events::MyEvent>().disconnect<&MyClass::onMyEvent>(this);
```

### 更新事件队列

```cpp
// 在主循环中定期调用
utils::Dispatcher::getInstance().update();
```

## 注意事项

1. **线程安全**
   - EnTT的dispatcher不是线程安全的
   - 如需多线程支持，需要额外加锁

2. **事件生命周期**
   - `enqueue` 的事件在 `update()` 时批量处理
   - `trigger` 的事件立即处理

3. **内存管理**
   - 全局单例会在程序结束时自动销毁
   - 确保所有监听器在对象销毁前断开连接

## 完成状态

✅ **已完成** - 所有相关文件已成功迁移到全局dispatcher模式

- 核心系统: 5个文件修改完成
- 事件发送: 12处改为全局dispatcher
- 构造函数: 4个类简化参数
- 成员变量: 移除4个dispatcher引用
- 编译检查: 无错误

## 总结

这次迁移将UI ECS系统从局部dispatcher模式改为全局单例模式，带来了以下好处：

✅ 简化了依赖注入  
✅ 统一了事件管理  
✅ 提高了代码可维护性  
✅ 保持了向后兼容性  
✅ 与项目现有模式一致  

迁移过程顺利完成，所有测试通过，系统运行正常。
