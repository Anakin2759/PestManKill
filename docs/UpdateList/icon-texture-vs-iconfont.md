# Icon 组件：纹理图标 vs 字体图标

## 概述

Icon 组件现在支持两种类型的图标加载方式：

1. **纹理图标（Texture Icon）** - 使用 PNG、JPG 等图像文件
2. **字体图标（IconFont）** - 使用 IconFont.ttf 字体文件，通过 Unicode 码点渲染

## 设计原理

### 为什么需要两种类型？

| 特性 | 纹理图标 | 字体图标 |
|------|---------|---------|
| **文件格式** | PNG, JPG, SVG | TTF, OTF |
| **加载方式** | 图像纹理 | 字体文件 |
| **缩放质量** | 依赖原始分辨率 | 矢量，无限缩放 |
| **颜色控制** | tintColor 叠加 | 完全由代码控制 |
| **文件大小** | 多个小文件 | 单个字体文件 |
| **使用场景** | 复杂彩色图标 | 单色矢量图标 |
| **常见库** | 自定义图标 | Font Awesome, Material Icons |

### 架构设计

```cpp
// 类型枚举
enum class IconType : uint8_t
{
    Texture,  // 纹理图标
    Font      // 字体图标
};

// Icon 组件定义
struct Icon
{
    IconType type;              // 图标类型
    
    // 纹理图标字段
    void* textureId;
    Vec2 uvMin, uvMax;
    
    // 字体图标字段
    void* fontHandle;
    uint32_t codepoint;
    
    // 通用字段
    Vec2 size;
    IconPosition position;
    float spacing;
    Color tintColor;
};
```

## API 使用

### 1. 纹理图标（推荐用于复杂图标）

```cpp
#include "ui/api/Factory.hpp"

// 加载图标纹理
void* iconTexture = LoadTexture("assets/icons/save.png");

// 创建按钮并设置纹理图标
auto btn = ui::factory::CreateButton("Save", "save_btn");
ui::factory::SetIconTexture(btn, iconTexture, 
                            policies::IconPosition::Left, 
                            20.0F,  // 大小
                            5.0F);  // 间距

// 或使用兼容的 SetIcon 函数
ui::factory::SetIcon(btn, iconTexture);  // 默认参数
```

### 2. 字体图标（推荐用于大量图标）

```cpp
#include "ui/api/Factory.hpp"

// 加载 IconFont 字体文件（只需加载一次）
TTF_Font* iconFont = TTF_OpenFont("assets/fonts/FontAwesome.ttf", 16);

// 创建按钮并设置字体图标
auto homeBtn = ui::factory::CreateButton("Home", "home_btn");
ui::factory::SetIconFont(homeBtn, 
                         iconFont, 
                         0xF015,  // Font Awesome 的 "home" 图标码点
                         policies::IconPosition::Left,
                         18.0F,
                         4.0F);

// 创建搜索按钮
auto searchBtn = ui::factory::CreateButton("Search", "search_btn");
ui::factory::SetIconFont(searchBtn, iconFont, 0xF002);  // "search" 图标
```

## 常见 IconFont 码点

### Font Awesome 5 (免费版)

```cpp
// 常用图标码点
constexpr uint32_t ICON_HOME      = 0xF015;  // 
constexpr uint32_t ICON_SEARCH    = 0xF002;  // 
constexpr uint32_t ICON_USER      = 0xF007;  // 
constexpr uint32_t ICON_SETTINGS  = 0xF013;  // ⚙
constexpr uint32_t ICON_HEART     = 0xF004;  // 
constexpr uint32_t ICON_STAR      = 0xF005;  // ★
constexpr uint32_t ICON_DOWNLOAD  = 0xF019;  // 
constexpr uint32_t ICON_UPLOAD    = 0xF093;  // 
constexpr uint32_t ICON_TRASH     = 0xF1F8;  // 
constexpr uint32_t ICON_EDIT      = 0xF044;  // ✎
constexpr uint32_t ICON_SAVE      = 0xF0C7;  // 💾
constexpr uint32_t ICON_CLOSE     = 0xF00D;  // ✕
constexpr uint32_t ICON_CHECK     = 0xF00C;  // ✓
constexpr uint32_t ICON_PLUS      = 0xF067;  // +
constexpr uint32_t ICON_MINUS     = 0xF068;  // -
constexpr uint32_t ICON_ARROW_LEFT  = 0xF060;  // ←
constexpr uint32_t ICON_ARROW_RIGHT = 0xF061;  // →
constexpr uint32_t ICON_ARROW_UP    = 0xF062;  // ↑
constexpr uint32_t ICON_ARROW_DOWN  = 0xF063;  // ↓
```

### Material Design Icons

```cpp
// Google Material Icons 码点
constexpr uint32_t ICON_HOME      = 0xE88A;
constexpr uint32_t ICON_SEARCH    = 0xE8B6;
constexpr uint32_t ICON_SETTINGS  = 0xE8B8;
constexpr uint32_t ICON_MENU      = 0xE5D2;
constexpr uint32_t ICON_CLOSE     = 0xE5CD;
constexpr uint32_t ICON_CHECK     = 0xE5CA;
```

## 完整示例

### 示例 1：工具栏（纹理图标）

```cpp
void CreateToolbarWithTextureIcons()
{
    auto toolbar = ui::factory::CreateHBoxLayout("toolbar");
    
    // 加载所有图标纹理
    void* saveIcon = LoadTexture("assets/icons/save.png");
    void* openIcon = LoadTexture("assets/icons/open.png");
    void* undoIcon = LoadTexture("assets/icons/undo.png");
    void* redoIcon = LoadTexture("assets/icons/redo.png");
    
    // 创建按钮并设置图标
    auto saveBtn = ui::factory::CreateButton("Save", "save_btn");
    ui::factory::SetIconTexture(saveBtn, saveIcon, policies::IconPosition::Left, 20.0F);
    ui::AddChild(toolbar, saveBtn);
    
    auto openBtn = ui::factory::CreateButton("Open", "open_btn");
    ui::factory::SetIconTexture(openBtn, openIcon, policies::IconPosition::Left, 20.0F);
    ui::AddChild(toolbar, openBtn);
    
    auto undoBtn = ui::factory::CreateButton("Undo", "undo_btn");
    ui::factory::SetIconTexture(undoBtn, undoIcon, policies::IconPosition::Left, 20.0F);
    ui::AddChild(toolbar, undoBtn);
    
    auto redoBtn = ui::factory::CreateButton("Redo", "redo_btn");
    ui::factory::SetIconTexture(redoBtn, redoIcon, policies::IconPosition::Left, 20.0F);
    ui::AddChild(toolbar, redoBtn);
}
```

### 示例 2：导航菜单（字体图标）

```cpp
void CreateNavigationWithFontIcons()
{
    // 加载 Font Awesome 字体（整个应用只需加载一次）
    static TTF_Font* fontAwesome = TTF_OpenFont("assets/fonts/FontAwesome.ttf", 16);
    
    auto menu = ui::factory::CreateVBoxLayout("nav_menu");
    
    // 使用字体图标的好处：只需一个字体文件
    auto homeBtn = ui::factory::CreateButton("Home", "home_btn");
    ui::factory::SetIconFont(homeBtn, fontAwesome, 0xF015, policies::IconPosition::Left, 16.0F);
    ui::AddChild(menu, homeBtn);
    
    auto searchBtn = ui::factory::CreateButton("Search", "search_btn");
    ui::factory::SetIconFont(searchBtn, fontAwesome, 0xF002, policies::IconPosition::Left, 16.0F);
    ui::AddChild(menu, searchBtn);
    
    auto userBtn = ui::factory::CreateButton("Profile", "user_btn");
    ui::factory::SetIconFont(userBtn, fontAwesome, 0xF007, policies::IconPosition::Left, 16.0F);
    ui::AddChild(menu, userBtn);
    
    auto settingsBtn = ui::factory::CreateButton("Settings", "settings_btn");
    ui::factory::SetIconFont(settingsBtn, fontAwesome, 0xF013, policies::IconPosition::Left, 16.0F);
    ui::AddChild(menu, settingsBtn);
}
```

### 示例 3：状态标签（混合使用）

```cpp
void CreateStatusLabels()
{
    TTF_Font* iconFont = TTF_OpenFont("assets/fonts/MaterialIcons.ttf", 14);
    
    // 成功状态 - 使用字体图标
    auto successLabel = ui::factory::CreateLabel("Operation Successful", "success_label");
    ui::factory::SetIconFont(successLabel, iconFont, 0xE5CA);  // check 图标
    if (auto* icon = Registry::TryGet<components::Icon>(successLabel))
    {
        icon->tintColor = {0.0F, 1.0F, 0.0F, 1.0F};  // 绿色
    }
    
    // 警告状态 - 使用纹理图标（彩色警告图标）
    void* warningTexture = LoadTexture("assets/icons/warning_color.png");
    auto warningLabel = ui::factory::CreateLabel("Warning", "warning_label");
    ui::factory::SetIconTexture(warningLabel, warningTexture);
    
    // 错误状态 - 使用字体图标
    auto errorLabel = ui::factory::CreateLabel("Error Occurred", "error_label");
    ui::factory::SetIconFont(errorLabel, iconFont, 0xE5CD);  // close 图标
    if (auto* icon = Registry::TryGet<components::Icon>(errorLabel))
    {
        icon->tintColor = {1.0F, 0.0F, 0.0F, 1.0F};  // 红色
    }
}
```

### 示例 4：动态切换图标类型

```cpp
void DynamicIconSwitch(entt::entity button, bool useFont)
{
    if (useFont)
    {
        // 切换到字体图标
        TTF_Font* iconFont = TTF_OpenFont("assets/fonts/FontAwesome.ttf", 16);
        ui::factory::SetIconFont(button, iconFont, 0xF04B);  // play 图标
    }
    else
    {
        // 切换到纹理图标
        void* playTexture = LoadTexture("assets/icons/play.png");
        ui::factory::SetIconTexture(button, playTexture);
    }
}
```

## 最佳实践

### 何时使用纹理图标

✅ **推荐场景**：

- 彩色或渐变图标
- 复杂的自定义设计
- 需要透明度效果的图标
- Logo 或品牌标识
- 少量图标的小型应用

### 何时使用字体图标

✅ **推荐场景**：

- 大量单色图标
- 需要动态改变颜色的图标
- 需要完美缩放的矢量图标
- 使用标准图标库（Font Awesome、Material Icons）
- 减少资源文件数量

### 性能对比

| 维度 | 纹理图标 | 字体图标 |
|------|---------|---------|
| **内存占用** | 每个图标占用纹理内存 | 所有图标共享字体纹理 |
| **加载时间** | 多个文件加载 | 单个字体文件 |
| **渲染性能** | 直接纹理渲染 | 需要文本渲染 |
| **缩放质量** | 放大可能模糊 | 矢量，完美缩放 |

### 推荐策略

```cpp
// 应用启动时加载字体图标
class IconManager
{
public:
    static void Initialize()
    {
        fontAwesome = TTF_OpenFont("assets/fonts/FontAwesome.ttf", 16);
        materialIcons = TTF_OpenFont("assets/fonts/MaterialIcons.ttf", 16);
    }
    
    static void Shutdown()
    {
        if (fontAwesome) TTF_CloseFont(fontAwesome);
        if (materialIcons) TTF_CloseFont(materialIcons);
    }
    
    static TTF_Font* GetFontAwesome() { return fontAwesome; }
    static TTF_Font* GetMaterialIcons() { return materialIcons; }
    
private:
    static TTF_Font* fontAwesome;
    static TTF_Font* materialIcons;
};

// 使用时
void CreateButton()
{
    auto btn = ui::factory::CreateButton("Home", "home_btn");
    ui::factory::SetIconFont(btn, 
                            IconManager::GetFontAwesome(), 
                            0xF015,
                            policies::IconPosition::Left);
}
```

## 渲染原理

### 纹理图标渲染

```cpp
// 直接作为纹理批次渲染
addImageBatch(textureId, pos, size, uvMin, uvMax, tintColor, alpha);
```

### 字体图标渲染

```cpp
// 将码点转换为 UTF-8 字符
uint32_t codepoint = 0xF015;
std::string utf8Char = ConvertCodepointToUTF8(codepoint);

// 临时切换字体渲染
TTF_Font* prevFont = currentFont;
currentFont = iconFont;
RenderText(utf8Char, pos, size, color, alpha);
currentFont = prevFont;
```

## 常见问题

### Q: 字体图标显示为方块？

A: 确保字体文件包含该码点，并且字体已正确加载。

### Q: 图标颜色不对？

A: 字体图标始终使用 `tintColor`，纹理图标通过 tintColor 叠加。

### Q: 如何获取 IconFont 的码点？

A: 查看字体的官方文档或使用字体编辑工具（如 FontForge）查看。

### Q: 可以混合使用吗？

A: 可以，不同控件可以使用不同类型的图标。

### Q: 如何创建自己的 IconFont？

A: 使用 IcoMoon、Fontello 等工具从 SVG 创建字体文件。

## 获取 IconFont 资源

### 免费资源

- **Font Awesome**: <https://fontawesome.com/>
- **Material Icons**: <https://fonts.google.com/icons>
- **Bootstrap Icons**: <https://icons.getbootstrap.com/>
- **Feather Icons**: <https://feathericons.com/>
- **Ionicons**: <https://ionic.io/ionicons>

### 工具

- **IcoMoon**: <https://icomoon.io/> (SVG 转 IconFont)
- **Fontello**: <https://fontello.com/> (定制图标字体)
- **FontForge**: 查看和编辑字体文件

## 总结

Icon 组件通过支持纹理和字体两种类型，提供了灵活的图标解决方案：

- ✅ **纹理图标**：适合复杂彩色图标
- ✅ **字体图标**：适合大量单色矢量图标
- ✅ **类型安全**：通过 `IconType` 枚举区分
- ✅ **易于使用**：提供专用 API 函数
- ✅ **性能优化**：根据类型选择最优渲染路径

根据你的具体需求选择合适的图标类型，或混合使用以获得最佳效果！
