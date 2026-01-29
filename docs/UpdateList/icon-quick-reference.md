# Icon 组件快速参考

## 核心 API

### 纹理图标

```cpp
void SetIconTexture(entt::entity entity,
                    void* textureId,
                    policies::IconPosition position = IconPosition::Left,
                    float iconSize = 16.0F,
                    float spacing = 4.0F);
```

### 字体图标

```cpp
void SetIconFont(entt::entity entity,
                 void* fontHandle,
                 uint32_t codepoint,
                 policies::IconPosition position = IconPosition::Left,
                 float iconSize = 16.0F,
                 float spacing = 4.0F);
```

### 移除图标

```cpp
void RemoveIcon(entt::entity entity);
```

## 枚举

### IconType

```cpp
enum class IconType : uint8_t
{
    Texture,  // 纹理图标（PNG、JPG等）
    Font      // 字体图标（IconFont.ttf）
};
```

### IconPosition

```cpp
enum class IconPosition : uint8_t
{
    Left,    // 图标在文本左侧
    Right,   // 图标在文本右侧
    Top,     // 图标在文本上方
    Bottom   // 图标在文本下方
};
```

## Icon 组件结构

```cpp
struct Icon
{
    IconType type;              // 图标类型
    
    // 纹理图标字段
    void* textureId;
    Vec2 uvMin{0.0F, 0.0F};
    Vec2 uvMax{1.0F, 1.0F};
    
    // 字体图标字段
    void* fontHandle;
    uint32_t codepoint;
    
    // 通用字段
    Vec2 size{16.0F, 16.0F};
    IconPosition position = IconPosition::Left;
    float spacing = 4.0F;
    Color tintColor{1.0F, 1.0F, 1.0F, 1.0F};
};
```

## 使用示例

### 纹理图标

```cpp
auto btn = ui::factory::CreateButton("Save", "save_btn");
void* icon = LoadTexture("save.png");
ui::factory::SetIconTexture(btn, icon);
```

### 字体图标

```cpp
auto btn = ui::factory::CreateButton("Home", "home_btn");
TTF_Font* font = TTF_OpenFont("FontAwesome.ttf", 16);
ui::factory::SetIconFont(btn, font, 0xF015);  // home icon
```

### 自定义颜色

```cpp
ui::factory::SetIconFont(btn, font, 0xF004);  // heart
if (auto* icon = Registry::TryGet<components::Icon>(btn))
{
    icon->tintColor = {1.0F, 0.0F, 0.0F, 1.0F};  // red
}
```

## 常用 Font Awesome 码点

```cpp
0xF015  // home 
0xF002  // search 
0xF007  // user 
0xF013  // settings ⚙
0xF004  // heart 
0xF005  // star ★
0xF019  // download 
0xF093  // upload 
0xF1F8  // trash 
0xF044  // edit ✎
0xF0C7  // save 💾
0xF00D  // close ✕
0xF00C  // check ✓
```

## 选择指南

| 场景 | 推荐 |
|------|------|
| 彩色图标 | Texture |
| 大量单色图标 | Font |
| 需要完美缩放 | Font |
| 复杂设计 | Texture |
| 标准图标库 | Font |
| Logo/品牌 | Texture |

## 获取资源

- Font Awesome: <https://fontawesome.com/>
- Material Icons: <https://fonts.google.com/icons>
- IcoMoon: <https://icomoon.io/> (自定义)
