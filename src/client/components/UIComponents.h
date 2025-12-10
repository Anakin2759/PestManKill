/**
 * ************************************************************************
 *
 * @file UIComponents.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief UI ECS 组件定义
 *
 * 所有UI元素的数据组件
 * 遵循ECS模式：纯数据结构，无行为逻辑，只包含属性
 * 不使用继承和回调函数，确保组件的纯粹性和独立性

  - 尺寸组件：管理尺寸属性
  - 位置组件：管理坐标属性
  - 滚动条组件：支持内容滚动和滚动条显示
  - 圆角组件：配置圆角效果
  - 父子关系组件：管理UI元素的层级关系，决定渲染顺序和位置计算
  - 布局组件：支持水平和垂直布局，包含间隔和对齐方式
  - 可见性组件：控制元素的显示与隐藏，以及透明度
  - 位置和尺寸组件：定义元素的坐标和大小，支持固定尺寸和自适应
  - 背景和圆角组件：支持背景颜色和圆角效果的配置
  - 文本显示组件：显示和存储文本内容和样式信息
  - 文本编辑器组件：支持单行和多行文本输入
  - tag组件：标记不同类型的UI元素（按钮、标签、文本框等）作为索引
  - 渲染状态组件：防止渲染重入，确保渲染过程安全
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <imgui.h>
#include <string>
#include <vector>
#include <functional>
#include <entt/entt.hpp>
#include <entt/core/type_traits.hpp>

namespace ui::components
{

// ===================== 基础组件 =====================
/**
 * @brief
 */
struct Size
{
    float width = 0.0F;
    float height = 0.0F;
    float minWidth = 0.0F;
    float minHeight = 0.0F;
    float maxWidth = FLT_MAX;
    float maxHeight = FLT_MAX;
    bool autoSize = true; // 是否自动根据内容调整大小
};

/**
 * @brief UI元素的位置组件
 */
struct Position
{
    float x = 0.0F;
    float y = 0.0F;

    bool useCustomPosition = false; // 是否使用自定义位置，默认为 false（由布局管理）
};

/**
 * @brief 可见性组件
 */
struct Visibility
{
    float alpha = 1.0F;
    bool isRendering = false;
    bool visible = true;
};

/**
 * @brief 背景绘制组件
 */
struct Background
{
    ImVec4 color{0.0F, 0.0F, 0.0F, 0.0F};
    bool enabled = false;
};
/**
 * @brief
 */
struct ScrollArea
{
    ImVec2 scrollOffset{0.0f, 0.0f}; // 当前滚动位置
    ImVec2 contentSize{0.0f, 0.0f};  // 内容区域大小
    bool horizontalScroll = false;   // 是否启用水平滚动
    bool verticalScroll = true;      // 是否启用垂直滚动
    bool showScrollbars = true;      // 是否显示滚动条

    // 滚动速度
    float scrollSpeed = 10.0f;
};

/**
 * @brief 圆角组件
 */
struct RoundedCorners
{
    float radius = 0.0F; // 圆角半径
};

/**
 * @brief 层级关系组件 - 父子关系
 */
struct Hierarchy
{
    entt::entity parent = entt::null;      // 父节点
    entt::entity firstChild = entt::null;  // 第一个子节点
    entt::entity nextSibling = entt::null; // 下一个兄弟节点
};

/**
 * @brief 可选中组件 (Checkable)
 */
struct Checkable
{
    bool checkable = false;
    bool checked = false;
};

/**
 * @brief 文本显示组件
 */
struct ShowText
{
    ImVec4 textColor{1.0F, 1.0F, 1.0F, 1.0F};
    bool wordWrap = false;
    float wrapWidth = 0.0F;
};
/**
 * @brief 点击信息组件
 */
struct Clickable
{
    bool enabled = true;
    bool useCustomColor = false;
};

// ===================== 布局组件 =====================

/**
 * @brief 布局容器组件
 */
struct LayoutInfo
{
    LayoutDirection direction = LayoutDirection::HORIZONTAL;
    float spacing = 5.0F;       // 元素间距
    ImVec4 margins{0, 0, 0, 0}; // 内边距
};

/**
 * @brief 间隔器组件
 */
struct Spacer
{
    uint8_t stretchFactor = 1; // 默认拉伸因子
    bool horizontal = true;    // 是否为水平间隔器
};

/**
 * @brief 文本编辑框组件
 */
struct TextEdit
{
    std::string text;
    std::string placeholder;
    std::string uniqueId;

    bool multiline = false;
    bool readOnly = false;
    bool password = false;
    size_t maxLength = 256;

    ImVec4 textColor{1.0F, 1.0F, 1.0F, 1.0F};
    ImVec4 backgroundColor{0.2F, 0.2F, 0.2F, 1.0F};
};

/**
 * @brief 图像组件
 */
struct Image
{
    void* textureId = nullptr;
    ImVec2 uvMin{0.0F, 0.0F};
    ImVec2 uvMax{1.0F, 1.0F};
    ImVec4 tintColor{1.0F, 1.0F, 1.0F, 1.0F};
    ImVec4 borderColor{0.0F, 0.0F, 0.0F, 0.0F};
    bool maintainAspectRatio = true;
};

/**
 * @brief 窗口组件
 */
struct Window
{
    std::string title;
    bool hasTitleBar = true;
    bool hasToolbar = false;
    bool modal = true;
    ImVec2 minSize{300.0F, 200.0F};
    ImVec2 maxSize{FLT_MAX, FLT_MAX};
};

/**
 * @brief 箭头组件
 */
struct Arrow
{
    ImVec2 startPoint{0.0F, 0.0F};
    ImVec2 endPoint{100.0F, 100.0F};
    ImVec4 color{1.0F, 1.0F, 1.0F, 1.0F};
    float thickness = 2.0F;
    float arrowSize = 10.0F;
};

/**
 * @brief 列表区域组件
 */
struct ListArea
{
    std::vector<entt::entity> items;
    int selectedIndex = -1;
    bool multiSelect = false;
    std::vector<int> selectedIndices;
    float itemHeight = 30.0F;
};

/**
 * @brief 表格组件
 */
struct TableInfo
{
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
    std::vector<float> columnWidths;
    bool resizable = true;
    bool sortable = false;
    int sortColumn = -1;
    bool sortAscending = true;
};

struct LineInfo
{
    ImVec2 startPoint{0.0F, 0.0F};
    ImVec2 endPoint{100.0F, 0.0F};
    ImVec4 color{1.0F, 1.0F, 1.0F, 1.0F};
    float thickness = 2.0F; // 线条粗细
};

// ===================== 动画组件 =====================

/**
 * @brief 动画时间组件
 */
struct Animation
{
    float duration = 1.0F; // 动画持续时间
    float elapsed = 0.0F;  // 已经过去的时间
};

struct AnimationPosition
{
    ImVec2 from; // 起始位置
    ImVec2 to;   // 目标位置
};

struct AnimationAlpha
{
    float from = 1.0F; // 起始透明度
    float to = 0.0F;   // 目标透明度
};

struct AnimationMode
{
    PlayMode mode = PlayMode::ONCE;
};

/**
 * @brief 动画缓动组件
 */
struct AnimationEase
{
    EasingType type = EasingType::Linear;
    // 可选：自定义曲线函数指针或 lambda
    std::function<float(float)> customCurve = nullptr;
};

// ===================== 样式组件 =====================
/**
 * @brief 样式组件
 */
struct Style
{
    struct Colors
    {
        ImVec4 text{1.0f, 1.0f, 1.0f, 1.0f};
        ImVec4 background{0.0f, 0.0f, 0.0f, 0.0f};
        ImVec4 border{0.0f, 0.0f, 0.0f, 0.0f};
        ImVec4 hover{0.0f, 0.0f, 0.0f, 0.0f};
        ImVec4 active{0.0f, 0.0f, 0.0f, 0.0f};
    };

    Colors colors;
    ImVec2 padding{0.0f, 0.0f};
    ImVec2 margin{0.0f, 0.0f};
    float borderRadius = 0.0f;
    ImVec2 borderSize{0.0f, 0.0f};

    // 标记哪些样式需要应用
    bool hasBackground = false;
    bool hasBorder = false;
};

// ===================== 交互与事件相关组件 =====================

/**
 * @brief 可选中/目标化组件
 * 标记某个 UI 元素或游戏实体为可作为目标。
 */
struct Targetable
{
    bool selectable = false; // 是否可被选中为目标
    int priority = 0;        // 目标优先级（UI 高亮/自动选择时使用）
};

// ===================== 渲染与状态组件 =====================
/**
 * @brief 渲染/状态组件，防止渲染重入与并发问题
 */
struct RenderGuard
{
    bool inRender = false; // 渲染过程中置为 true，避免重入
};

struct ButtonState
{
    ButtonVisualState visual = ButtonVisualState::Idle;
    bool triggered = false; // 本帧是否触发过动作
};
/**
 * @brief 标题组件
 */
struct Title
{
    std::string text;
};

} // namespace ui::components
