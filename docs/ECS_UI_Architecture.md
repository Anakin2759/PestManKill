# UI系统ECS架构设计文档

## 概述

本文档描述了PestManKill项目中UI系统的完全ECS（Entity-Component-System）化改造。新架构基于EnTT库，实现了高性能、灵活且可维护的UI框架。

## 架构设计

### 核心原则

1. **数据与行为分离**：组件只包含数据，系统负责处理逻辑
2. **组合优于继承**：通过组合组件实现功能而非类继承
3. **缓存友好**：组件数据连续存储，提高缓存命中率
4. **事件驱动**：使用EnTT的dispatcher进行松耦合通信

### 三层架构

```
┌─────────────────────────────────────────┐
│           应用层 (Application)           │
│      ECSApplication / MainWindowECS     │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│          管理层 (UiSystem)               │
│   Registry + Dispatcher + Factory       │
└─────────┬───────────────┬───────────────┘
          │               │
┌─────────▼─────┐  ┌─────▼──────────────┐
│  组件层        │  │   系统层            │
│  Components   │  │   Systems           │
│               │  │                     │
│ - Position    │  │ - UIRenderSystem    │
│ - Size        │  │ - UIAnimationSystem │
│ - Button      │  │                     │
│ - Label       │  │                     │
│ - Layout      │  │                     │
│ - ...         │  │                     │
└───────────────┘  └─────────────────────┘
```

## 组件设计

### 组件分类

#### 1. 基础组件 (Basic Components)

位于 `src/client/components/UIComponents.h`

- **Position**：位置信息 (x, y, useCustomPosition)
- **Size**：尺寸信息 (width, height, min/max, useFixedSize)
- **Visibility**：可见性 (visible, alpha)
- **Background**：背景绘制 (color, enabled)
- **Hierarchy**：父子关系 (parent, children)
- **RenderState**：渲染状态 (isRendering - 防重入)
- **Checkable**：可选中状态 (checkable, checked)

#### 2. 布局组件 (Layout Components)

- **Layout**：布局容器
  - `direction`: 布局方向 (HORIZONTAL/VERTICAL)
  - `items`: 布局项列表 (entity + stretchFactor + alignment)
  - `spacing`: 组件间距
  - `margins`: 边距 (left, top, right, bottom)

- **Spacer**：弹性间隔器
  - `stretchFactor`: 拉伸因子
  - `isHorizontal`: 方向

#### 3. 交互组件 (Interactive Components)

- **Button**：按钮
  - `text`, `tooltip`, `uniqueId`
  - `onClick`: 回调函数
  - `enabled`: 启用状态
  - `buttonColor`, `hoverColor`, `activeColor`

- **Label**：文本标签
  - `text`: 文本内容
  - `textColor`: 文本颜色
  - `wordWrap`: 自动换行
  - `wrapWidth`: 换行宽度

- **TextEdit**：文本编辑框
  - `text`, `placeholder`, `uniqueId`
  - `multiline`, `readOnly`, `password`
  - `maxLength`: 最大长度
  - `onTextChanged`: 回调函数

- **Image**：图像
  - `textureId`: 纹理指针
  - `uvMin`, `uvMax`: UV坐标
  - `tintColor`, `borderColor`
  - `maintainAspectRatio`

- **Dialog**：对话框
  - `title`, `isOpen`, `modal`
  - `minSize`, `maxSize`

- **Arrow**：箭头
  - `startPoint`, `endPoint`
  - `color`, `thickness`, `arrowSize`

- **ListArea**：列表区域
  - `items`: 项列表
  - `selectedIndex`, `selectedIndices`
  - `multiSelect`, `itemHeight`

- **Table**：表格
  - `headers`, `rows`, `columnWidths`
  - `resizable`, `sortable`
  - `sortColumn`, `sortAscending`

#### 4. 动画组件 (Animation Components)

- **Animation**：基础动画
  - `active`: 是否激活
  - `duration`: 持续时间
  - `elapsed`: 已流逝时间
  - `updateCallback`: 更新回调

- **PositionAnimation**：位置动画
  - `startPos`, `endPos`, `progress`

- **AlphaAnimation**：透明度动画
  - `startAlpha`, `endAlpha`, `progress`

#### 5. 标签组件 (Tag Components)

用于快速识别实体类型（无数据）：

- `WidgetTag`, `LayoutTag`, `ButtonTag`, `LabelTag`
- `TextEditTag`, `ImageTag`, `DialogTag`, `ArrowTag`
- `ListAreaTag`, `TableTag`, `SpacerTag`, `ApplicationTag`

## 系统设计

### UIRenderSystem

**职责**：渲染所有可见的UI元素

**核心方法**：

```cpp
void update()                                    // 渲染所有顶层元素
void renderWidget(entt::entity entity)           // 递归渲染元素及子元素
void renderButton(...)                           // 渲染按钮
void renderLabel(...)                            // 渲染标签
void renderTextEdit(...)                         // 渲染文本框
void renderImage(...)                            // 渲染图像
void renderArrow(...)                            // 渲染箭头
void renderLayout(...)                           // 渲染布局
void renderHorizontalLayout(...)                 // 水平布局算法
void renderVerticalLayout(...)                   // 垂直布局算法
```

**渲染流程**：

1. 遍历所有顶层元素（parent == null）
2. 对每个元素：
   - 检查可见性
   - 防止重入渲染
   - 渲染背景
   - 根据组件类型调用对应渲染函数
   - 递归渲染子元素
3. 布局元素特殊处理：
   - 计算固定尺寸和弹性空间
   - 分配空间给子元素
   - 更新子元素位置和尺寸
   - 渲染子元素

### UIAnimationSystem

**职责**：更新所有活动动画

**核心方法**：

```cpp
void update(float deltaTime)  // 更新所有动画
```

**更新流程**：

1. 遍历所有 `Animation` 组件
2. 更新已流逝时间
3. 计算进度 (0.0 ~ 1.0)
4. 调用 `updateCallback`
5. 处理专门的动画类型：
   - `PositionAnimation`：插值计算位置
   - `AlphaAnimation`：插值计算透明度
6. 动画完成时触发 `AnimationComplete` 事件

## 工厂模式

### UIFactory

**职责**：提供便捷的UI元素创建方法

**核心方法**：

```cpp
// 创建UI元素
entt::entity createButton(text, onClick)
entt::entity createLabel(text)
entt::entity createTextEdit(placeholder, multiline)
entt::entity createImage(textureId)
entt::entity createArrow(start, end)
entt::entity createHBoxLayout()
entt::entity createVBoxLayout()
entt::entity createSpacer(stretch, horizontal)
entt::entity createDialog(title)

// 层次结构管理
void addChild(parent, child)
void removeChild(parent, child)

// 布局管理
void addWidgetToLayout(layout, widget, stretch, alignment)
void addStretchToLayout(layout, stretch)

// 销毁
void destroyWidget(entity)  // 递归销毁子元素
```

**创建流程**：

1. 创建实体：`registry.create()`
2. 添加基础组件：Position, Size, Visibility, RenderState
3. 添加类型标签：ButtonTag, LabelTag, etc.
4. 添加功能组件：Button, Label, etc.
5. 初始化组件数据
6. 触发 `WidgetCreated` 事件
7. 返回实体ID

## 事件系统

### 事件类型

位于 `src/client/events/UIEvents.h`

- **RenderRequest**：渲染请求
- **ButtonClicked**：按钮点击
- **TextChanged**：文本改变
- **SelectionChanged**：选择改变
- **LayoutUpdate**：布局更新
- **AnimationComplete**：动画完成
- **WidgetCreated**：元素创建
- **WidgetDestroyed**：元素销毁
- **VisibilityChanged**：可见性改变

### 事件使用

```cpp
// 发送事件
dispatcher.enqueue<events::ButtonClicked>(entity);

// 监听事件
dispatcher.sink<events::ButtonClicked>().connect<&MyClass::onButtonClick>(this);

// 处理事件队列
dispatcher.update();
```

## 辅助工具

### UIHelper

位于 `src/client/utils/UIHelper.h`

提供便捷的组件操作函数：

```cpp
// 设置属性
setFixedSize(registry, entity, width, height)
setPosition(registry, entity, x, y)
setVisible(registry, entity, visible)
setAlpha(registry, entity, alpha)
setBackgroundColor(registry, entity, color)

// 布局操作
setLayoutSpacing(registry, entity, spacing)
setLayoutMargins(registry, entity, margins)

// 组件特定操作
setButtonText(registry, entity, text)
setButtonEnabled(registry, entity, enabled)
setLabelText(registry, entity, text)
setTextColor(registry, entity, color)
getTextEditContent(registry, entity)
setTextEditContent(registry, entity, text)

// 动画操作
startPositionAnimation(registry, entity, start, end, duration)
startAlphaAnimation(registry, entity, startAlpha, endAlpha, duration)
stopAnimation(registry, entity)
```

## 性能优化

### 1. 内存布局优化

- **组件数据连续存储**：EnTT保证同类型组件内存连续
- **缓存友好访问**：通过view批量访问组件
- **减少虚函数调用**：不使用继承，直接访问数据

### 2. 渲染优化

- **可见性剔除**：不渲染不可见元素
- **脏标记机制**：可扩展（布局变化时标记）
- **批量处理**：view遍历同类型组件

### 3. 事件优化

- **事件队列**：enqueue推迟事件处理
- **松耦合通信**：避免直接依赖
- **sink连接**：高效的事件分发

## 与旧系统的对比

| 特性 | 旧系统 (OOP) | 新系统 (ECS) |
|------|-------------|-------------|
| 架构模式 | 继承层次结构 | 组合组件 |
| 数据存储 | 分散在对象中 | 连续存储 |
| 行为实现 | 虚函数 | 系统处理 |
| 扩展性 | 需要修改类 | 添加组件/系统 |
| 性能 | 虚函数开销 | 直接访问数据 |
| 耦合度 | 紧耦合 | 松耦合 |
| 内存效率 | 碎片化 | 缓存友好 |

## 使用示例

### 简单按钮

```cpp
// 创建
auto button = factory.createButton("Click Me", []() {
    utils::LOG_INFO("Clicked!");
});

// 配置
helper::setFixedSize(registry, button, 200, 50);
helper::setPosition(registry, button, 100, 100);

// 添加到布局
factory.addWidgetToLayout(layout, button);
```

### 复杂布局

```cpp
// 主布局
auto mainLayout = factory.createVBoxLayout();
helper::setLayoutMargins(registry, mainLayout, 20);

// 标题区
auto titleLabel = factory.createLabel("Title");
factory.addWidgetToLayout(mainLayout, titleLabel, 0);

// 内容区（水平布局）
auto contentLayout = factory.createHBoxLayout();
factory.addWidgetToLayout(mainLayout, contentLayout, 1);

// 左侧按钮
auto leftButton = factory.createButton("Left");
factory.addWidgetToLayout(contentLayout, leftButton, 1);

// 弹性空间
factory.addStretchToLayout(contentLayout, 2);

// 右侧按钮
auto rightButton = factory.createButton("Right");
factory.addWidgetToLayout(contentLayout, rightButton, 1);
```

### 动画效果

```cpp
// 淡入动画
helper::setAlpha(registry, widget, 0.0F);
helper::startAlphaAnimation(registry, widget, 0.0F, 1.0F, 1.0F);

// 移动动画
ImVec2 start{100, 100};
ImVec2 end{300, 200};
helper::startPositionAnimation(registry, widget, start, end, 2.0F);

// 监听完成
dispatcher.sink<events::AnimationComplete>().connect<[](const auto& e) {
    utils::LOG_INFO("Animation complete!");
}>();
```

## 扩展指南

### 添加新组件

1. 在 `UIComponents.h` 中定义组件结构
2. 添加对应的Tag组件
3. 在 `UIFactory` 中添加创建方法
4. 在 `UIRenderSystem` 中添加渲染逻辑
5. 在 `UIHelper` 中添加便捷操作函数

### 添加新系统

1. 创建系统类，接收 `registry` 和 `dispatcher`
2. 实现 `update()` 方法
3. 在 `UiSystem` 中实例化系统
4. 在 `UiSystem::update()` 中调用系统更新

### 添加新事件

1. 在 `UIEvents.h` 中定义事件结构
2. 在适当位置 `enqueue` 事件
3. 在需要的地方 `connect` 监听器

## 文件结构

```
src/client/
├── components/
│   └── UIComponents.h          # 所有UI组件定义
├── events/
│   └── UIEvents.h              # UI事件定义
├── systems/
│   ├── UIRenderSystem.h        # 渲染系统
│   └── UIAnimationSystem.h     # 动画系统
├── model/
│   └── UiSystem.h              # UI系统管理器
├── utils/
│   ├── UIFactory.h             # UI工厂
│   └── UIHelper.h              # 辅助函数
├── ui/
│   └── ECSApplication.h        # ECS应用程序基类
├── view/
│   └── MainWindowECS.h         # 示例主窗口
└── main_ecs_ui_test.cpp        # 测试程序
```

## 迁移指南

### 从旧系统迁移

1. **替换Application基类**

   ```cpp
   // 旧代码
   class MyApp : public ui::Application { ... };
   
   // 新代码
   class MyApp : public ui::ECSApplication { ... };
   ```

2. **改变UI创建方式**

   ```cpp
   // 旧代码
   auto button = std::make_shared<ui::Button>("Text");
   button->setFixedSize(200, 50);
   
   // 新代码
   auto button = factory.createButton("Text");
   helper::setFixedSize(registry, button, 200, 50);
   ```

3. **改变布局添加方式**

   ```cpp
   // 旧代码
   layout->addWidget(button, stretch);
   
   // 新代码
   factory.addWidgetToLayout(layout, button, stretch);
   ```

4. **改变事件处理**

   ```cpp
   // 旧代码
   button->setOnClick([]() { ... });
   
   // 新代码（方式1：创建时指定）
   auto button = factory.createButton("Text", []() { ... });
   
   // 新代码（方式2：后续设置）
   auto& buttonComp = registry.get<components::Button>(button);
   buttonComp.onClick = []() { ... };
   
   // 新代码（方式3：监听事件）
   dispatcher.sink<events::ButtonClicked>().connect<handler>(this);
   ```

## 最佳实践

1. **始终使用Factory创建UI元素**，不要直接操作registry
2. **使用Helper函数修改组件**，保持代码简洁
3. **事件优于回调**，实现松耦合
4. **检查实体有效性**，使用 `registry.valid(entity)`
5. **避免长期持有组件引用**，每次使用时重新获取
6. **使用Tag组件**，快速识别实体类型
7. **批量操作使用View**，提高性能
8. **动画使用AnimationSystem**，不要手动计算

## 注意事项

1. 组件应该是纯数据结构（POD），不包含复杂逻辑
2. 系统应该是无状态的，所有状态存储在组件中
3. 使用 `entt::null` 表示空实体，不要使用 `0` 或 `-1`
4. 销毁UI元素必须使用 `factory.destroyWidget()`，会递归清理子元素
5. 事件处理需要调用 `dispatcher.update()`，通常在主循环中
6. 布局计算在渲染时进行，修改布局后会在下一帧生效
7. 组件回调函数（onClick等）应该捕获智能指针而非裸指针

## 总结

ECS化的UI系统具有以下优势：

✅ **高性能**：缓存友好的数据布局，减少虚函数调用  
✅ **灵活性**：通过组合组件实现任意功能  
✅ **可维护性**：清晰的职责分离，易于理解和修改  
✅ **可扩展性**：添加新功能无需修改现有代码  
✅ **类型安全**：编译期检查，减少运行时错误  
✅ **松耦合**：事件驱动通信，组件间无直接依赖  

这套架构为PestManKill项目提供了一个现代化、高性能的UI基础设施。
