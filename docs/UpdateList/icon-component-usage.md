# Icon 组件使用指南

## 概述

Icon 组件是一个装饰性组件，用于为 Button、Label 等控件添加图标。Icon 不是独立的控件（Entity），而是一个可以附加到现有控件上的组件（Component）。

## 设计原理

### Icon vs Image

| 特性 | Image 控件 | Icon 组件 |
|------|-----------|----------|
| **类型** | 独立的 Entity | 附加的 Component |
| **用途** | 展示图片内容 | 装饰其他控件 |
| **尺寸** | 可变、较大 | 固定、较小（默认16x16） |
| **独立性** | 独立存在 | 依附于宿主控件 |
| **交互** | 可独立交互 | 跟随宿主控件 |
| **布局** | 独立参与布局 | 自动根据文本位置计算 |

### 架构优势

1. **符合 ECS 模式**：Icon 是控件的属性，不是独立实体
2. **灵活性**：任何控件都可以添加/移除 Icon
3. **性能**：避免创建不必要的 Entity
4. **易于管理**：Icon 的生命周期与宿主控件绑定

## 组件定义

```cpp
struct Icon
{
    using is_component_tag = void;
    
    void* textureId = nullptr;                                 // 图标纹理句柄
    Vec2 size{16.0F, 16.0F};                                   // 图标尺寸
    policies::IconPosition position = policies::IconPosition::Left; // 相对于文本的位置
    float spacing = 4.0F;                                      // 与文本的间距
    Color tintColor{1.0F, 1.0F, 1.0F, 1.0F};                   // 图标颜色
    Vec2 uvMin{0.0F, 0.0F};                                    // UV 最小坐标
    Vec2 uvMax{1.0F, 1.0F};                                    // UV 最大坐标
};
```

## 图标位置枚举

```cpp
enum class IconPosition : uint8_t
{
    Left,   // 图标在文本左侧
    Right,  // 图标在文本右侧
    Top,    // 图标在文本上方
    Bottom  // 图标在文本下方
};
```

## API 函数

### SetIcon

为控件添加或更新图标：

```cpp
void SetIcon(entt::entity entity,
             void* textureId,
             policies::IconPosition position = policies::IconPosition::Left,
             float iconSize = 16.0F,
             float spacing = 4.0F);
```

### RemoveIcon

移除控件的图标：

```cpp
void RemoveIcon(entt::entity entity);
```

## 使用示例

### 1. 创建带图标的按钮

```cpp
#include "ui/api/Factory.hpp"

// 创建保存按钮
auto saveBtn = ui::factory::CreateButton("Save", "save_button");

// 加载图标纹理（假设你有纹理加载函数）
void* saveIconTexture = LoadTexture("assets/icons/save.png");

// 为按钮添加图标（默认在左侧，16x16，间距4px）
ui::factory::SetIcon(saveBtn, saveIconTexture);

// 或者指定位置和大小
ui::factory::SetIcon(saveBtn, 
                     saveIconTexture, 
                     policies::IconPosition::Left, 
                     20.0F,  // 图标大小
                     6.0F);  // 间距
```

### 2. 图标在不同位置

```cpp
// 图标在右侧
auto nextBtn = ui::factory::CreateButton("Next", "next_button");
ui::factory::SetIcon(nextBtn, nextIconTexture, policies::IconPosition::Right);

// 图标在上方
auto uploadBtn = ui::factory::CreateButton("Upload", "upload_button");
ui::factory::SetIcon(uploadBtn, uploadIconTexture, policies::IconPosition::Top);

// 图标在下方
auto downloadBtn = ui::factory::CreateButton("Download", "download_button");
ui::factory::SetIcon(downloadBtn, downloadIconTexture, policies::IconPosition::Bottom);
```

### 3. 为 Label 添加图标

```cpp
auto statusLabel = ui::factory::CreateLabel("Connected", "status_label");
void* statusIconTexture = LoadTexture("assets/icons/check.png");
ui::factory::SetIcon(statusLabel, statusIconTexture, policies::IconPosition::Left, 12.0F);
```

### 4. 自定义图标颜色

```cpp
auto errorBtn = ui::factory::CreateButton("Error", "error_button");
ui::factory::SetIcon(errorBtn, warningIconTexture);

// 修改图标颜色为红色
if (auto* icon = Registry::TryGet<components::Icon>(errorBtn))
{
    icon->tintColor = {1.0F, 0.0F, 0.0F, 1.0F}; // 红色
}
```

### 5. 动态修改图标

```cpp
auto playBtn = ui::factory::CreateButton("Play", "play_button");
void* playIconTexture = LoadTexture("assets/icons/play.png");
void* pauseIconTexture = LoadTexture("assets/icons/pause.png");

// 初始显示播放图标
ui::factory::SetIcon(playBtn, playIconTexture);

// 点击后切换为暂停图标
if (auto* icon = Registry::TryGet<components::Icon>(playBtn))
{
    icon->textureId = pauseIconTexture;
}
```

### 6. 移除图标

```cpp
auto btn = ui::factory::CreateButton("Button", "my_button");
ui::factory::SetIcon(btn, iconTexture);

// 后续移除图标
ui::factory::RemoveIcon(btn);
```

### 7. 使用 UV 坐标（图标集）

如果你的图标都在一张纹理图集（Atlas）中，可以使用 UV 坐标：

```cpp
auto btn = ui::factory::CreateButton("Icon", "icon_button");
void* atlasTexture = LoadTexture("assets/icons/atlas.png");
ui::factory::SetIcon(btn, atlasTexture);

// 设置 UV 坐标以显示图集中的特定图标
if (auto* icon = Registry::TryGet<components::Icon>(btn))
{
    // 假设图标在图集的左上角，占据 1/4 的区域
    icon->uvMin = {0.0F, 0.0F};
    icon->uvMax = {0.5F, 0.5F};
}
```

## 渲染原理

Icon 的渲染逻辑集成在 RenderSystem 中：

1. **触发时机**：当渲染带有 Text 组件的控件（Button、Label 等）时
2. **位置计算**：根据 `IconPosition` 和文本尺寸自动计算图标位置
3. **居中对齐**：图标和文本会自动在控件内居中对齐
4. **透明度继承**：图标继承宿主控件的透明度

### 位置计算逻辑

```cpp
// 示例：图标在左侧时的位置计算
float totalWidth = iconWidth + spacing + textWidth;
iconPos.x = widgetPos.x + (widgetWidth - totalWidth) / 2;
iconPos.y = widgetPos.y + (widgetHeight - iconHeight) / 2;
```

## 注意事项

1. **纹理管理**：Icon 组件只存储纹理指针，不负责纹理的加载和释放
2. **性能**：图标与文本在同一渲染批次中，性能开销很小
3. **布局影响**：当前实现中，Icon 不影响控件的自动尺寸计算（可根据需要扩展）
4. **支持控件**：目前支持 Button、Label、Text 等带有 Text 组件的控件

## 扩展建议

### 1. 支持多个图标

如果需要同时显示前缀和后缀图标，可以扩展为：

```cpp
struct Icon
{
    std::vector<IconData> icons; // 支持多个图标
};

struct IconData
{
    void* textureId;
    policies::IconPosition position;
    Vec2 size;
    // ...
};
```

### 2. 图标动画

可以添加动画相关的字段：

```cpp
struct Icon
{
    // ... 现有字段
    float rotation = 0.0F;           // 旋转角度
    bool animating = false;          // 是否正在动画
    policies::AnimationType animType; // 动画类型
};
```

### 3. 自动调整控件尺寸

在 LayoutSystem 中添加逻辑，让带有 Icon 的控件自动调整尺寸：

```cpp
if (auto* icon = Registry::TryGet<components::Icon>(entity))
{
    if (icon->position == IconPosition::Left || icon->position == IconPosition::Right)
    {
        autoWidth += icon->size.x + icon->spacing;
    }
    else
    {
        autoHeight += icon->size.y + icon->spacing;
    }
}
```

## 完整示例

```cpp
#include "ui/api/Factory.hpp"
#include "ui/singleton/Registry.hpp"

void CreateToolbar()
{
    auto toolbar = ui::factory::CreateHBoxLayout("toolbar");
    
    // 加载图标纹理
    void* saveIcon = LoadTexture("assets/icons/save.png");
    void* openIcon = LoadTexture("assets/icons/open.png");
    void* closeIcon = LoadTexture("assets/icons/close.png");
    
    // 创建保存按钮
    auto saveBtn = ui::factory::CreateButton("Save", "save_btn");
    ui::factory::SetIcon(saveBtn, saveIcon, policies::IconPosition::Left, 18.0F);
    ui::AddChild(toolbar, saveBtn);
    
    // 创建打开按钮
    auto openBtn = ui::factory::CreateButton("Open", "open_btn");
    ui::factory::SetIcon(openBtn, openIcon, policies::IconPosition::Left, 18.0F);
    ui::AddChild(toolbar, openBtn);
    
    // 创建关闭按钮（图标在右侧）
    auto closeBtn = ui::factory::CreateButton("Close", "close_btn");
    ui::factory::SetIcon(closeBtn, closeIcon, policies::IconPosition::Right, 18.0F);
    
    // 自定义关闭按钮图标颜色为红色
    if (auto* icon = Registry::TryGet<components::Icon>(closeBtn))
    {
        icon->tintColor = {1.0F, 0.2F, 0.2F, 1.0F};
    }
    ui::AddChild(toolbar, closeBtn);
}
```

## 总结

Icon 组件采用组件化设计，具有以下优点：

- ✅ 符合 ECS 架构理念
- ✅ 灵活且易于使用
- ✅ 性能开销小
- ✅ 自动位置计算
- ✅ 支持自定义样式

这种设计使得 Icon 成为 UI 框架中一个强大而优雅的功能。
