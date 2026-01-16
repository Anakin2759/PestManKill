/**
 * ************************************************************************
 *
 * @file Components.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @brief UI ECS 组件定义 (完整且优化版)
 *
 * 遵循ECS模式：纯数据结构，无行为逻辑，只包含属性,不含状态
 * 确保组件的纯粹性和独立性
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include "Types.h"
#include <string>
#include <vector>
#include <functional>
#include <cfloat>
#include <entt/entt.hpp>
#include "Policies.h"
#include <SDL3/SDL.h>
namespace ui::components
{

// ===================== 基本信息组件 =====================
struct BaseInfo
{
    std::string alias; // 组件别名，便于调试和识别
};
// ===================== 基础尺寸与位置 =====================

/**
 * @brief 尺寸约束组件
 */
struct Size
{
    Vec2 size{0.0F, 0.0F};
    Vec2 minSize{0.0F, 0.0F};
    Vec2 maxSize{FLT_MAX, FLT_MAX};
    bool autoSize = true;                               // 是否自动根据内容调整大小 (兼容旧代码)
    policies::Size widthPolicy = policies::Size::Auto;  // 宽度策略
    policies::Size heightPolicy = policies::Size::Auto; // 高度策略
};

/**
 * @brief UI 元素的相对位置组件
 */
struct Position
{
    Vec2 value{0.0F, 0.0F};
};

/**
 * @brief UI画布/屏幕尺寸（由渲染系统在 ImGui 帧内更新）
 *
 * 该组件推荐存放在 registry.ctx() 中，供 LayoutSystem 等逻辑系统读取，
 * 以避免直接依赖 ImGui::GetIO().DisplaySize。
 */
struct CanvasSize
{
    Vec2 value{0.0F, 0.0F};
};

/**
 * @brief 边距组件 (外边距)
 */
struct Margin
{
    Vec4 values{0.0F, 0.0F, 0.0F, 0.0F}; // Top, Right, Bottom, Left
};

/**
 * @brief 内边距组件
 * 定义元素内容与其边框之间的空间。
 */
struct Padding
{
    Vec4 values{0.0F, 0.0F, 0.0F, 0.0F}; // Top, Right, Bottom, Left
};

/**
 * @brief 背景绘制组件
 */
struct Background
{
    Color color{0.0F, 0.0F, 0.0F, 0.0F};
    Vec4 borderRadius{0.0F, 0.0F, 0.0F, 0.0F}; // 圆角半径 (x:TopLeft, y:TopRight, z:BottomRight, w:BottomLeft)
    bool enabled = false;
};

/**
 * @brief 边框组件
 */
struct Border
{
    Color color{1.0F, 1.0F, 1.0F, 1.0F};
    float thickness = 1.0F;
    Vec4 borderRadius{0.0F, 0.0F, 0.0F, 0.0F}; // 圆角半径
    bool enabled = false;
};

/**
 * @brief 阴影组件
 */
struct Shadow
{
    float softness{};                    // 阴影柔和度
    Vec2 offset{0.0F, 0.0F};             // 阴影偏移 (x, y)
    Color color{0.0F, 0.0F, 0.0F, 1.0F}; // 阴影颜色
    bool enabled = false;
};

/**
 * @brief 透明度组件
 */
struct Alpha
{
    float value = 1.0F;
};

// ===================== 层级与滚动 =====================

/**
 * @brief 层级关系组件 - Parent-Children 结构
 */
struct Hierarchy
{
    entt::entity parent = entt::null;
    std::vector<entt::entity> children; // 存储所有子节点
};

/**
 * @brief 滚动区域组件
 */
struct ScrollArea
{
    static constexpr float DEFAULT_SCROLL_SPEED = 10.0F;
    Vec2 scrollOffset{0.0F, 0.0F};           // 当前滚动位置
    Vec2 contentSize{0.0F, 0.0F};            // 内容区域大小
    float scrollSpeed{DEFAULT_SCROLL_SPEED}; // 滚动速度
    bool horizontalScroll = false;
    bool verticalScroll = true;
    bool showScrollbars = true;
};

// ===================== 布局组件 =====================

/**
 * @brief 布局容器配置组件
 */
struct LayoutInfo
{
    static constexpr float DEFAULT_SPACING = 5.0F;
    policies::LayoutDirection direction = policies::LayoutDirection::HORIZONTAL;
    float spacing = DEFAULT_SPACING; // 元素间距
};

/**
 * @brief 间隔器配置组件
 */
struct Spacer
{
    uint8_t stretchFactor = 1; // 默认拉伸因子
};

// ===================== 文本组件 =====================

/**
 * @brief 文本内容组件
 */
struct Text
{
    std::string content;
    Color color{1.0F, 1.0F, 1.0F, 1.0F};
    float fontSize = 0.0F; // 0 表示使用 ImGui 默认字体大小

    float wrapWidth = 0.0F;
    bool wordWrap = false;
    policies::Alignment alignment = policies::Alignment::NONE;
};

/**
 * @brief 文本编辑框数据组件
 */
struct TextEdit
{
    static constexpr size_t MAX_LENGTH = 1024;
    std::string buffer; // 存储输入文本的缓冲区
    std::string placeholder;
    Color textColor{1.0F, 1.0F, 1.0F, 1.0F};
    size_t maxLength = MAX_LENGTH;
    bool multiline = false;
    bool readOnly = false;
    bool password = false;
};

// ===================== 图像组件 =====================

/**
 * @brief 图像组件
 */
struct Image
{
    void* textureId = nullptr;                 // 纹理句柄 (例如 SDL_Texture* 或 OpenGL ID)
    Vec2 uvMin{0.0F, 0.0F};                    // UV 最小坐标
    Vec2 uvMax{1.0F, 1.0F};                    // UV 最大坐标
    Color tintColor{1.0F, 1.0F, 1.0F, 1.0F};   // 颜色叠加
    Color borderColor{0.0F, 0.0F, 0.0F, 0.0F}; // 边框颜色
    bool maintainAspectRatio = true;           // 是否保持宽高比
};

// ===================== 交互与状态 =====================

/**
 * @brief 可点击组件
 */
struct Clickable
{
    std::move_only_function<void()> onClick;
    bool enabled = true;
};

/**
 * @brief 可悬浮组件
 */
struct Hoverable
{
    std::move_only_function<void()> onHover;
    std::move_only_function<void()> onUnhover;
    bool enabled = true;
};

/**
 * @brief 可按压组件 - 用于处理长按、拖动等场景
 */
struct Pressable
{
    std::move_only_function<void()> onPress;   // 鼠标按下时触发
    std::move_only_function<void()> onRelease; // 鼠标松开时触发
    bool enabled = true;
};

/**
 * @brief 可选中组件
 */
struct Checkable
{
    bool checked = false;
};

/**
 * @brief 按钮状态组件
 */
struct ButtonState
{
    policies::ButtonVisual visual = policies::ButtonVisual::Idle;
    bool triggered = false; // 本帧是否触发过动作
};

// ===================== 动画组件 =====================

/**
 * @brief 动画时间状态
 */
struct AnimationTime
{
    float duration = 1.0F;
    float elapsed = 0.0F;
    policies::Easing easing = policies::Easing::Linear;
    policies::Play mode = policies::Play::ONCE;
};

/**
 * @brief 位置动画目标
 */
struct AnimationPosition
{
    Vec2 from;
    Vec2 to;
};

/**
 * @brief 透明度动画目标
 */
struct AnimationAlpha
{
    float from = 1.0F;
    float to = 0.0F;
};

// ===================== 复杂组件数据 =====================

/**
 * @brief 窗口组件
 */
struct Window
{
    static constexpr float MIN_WID = 300.0F;
    static constexpr float MIN_HIG = 200.0F;
    std::string title;
    Vec2 minSize{MIN_WID, MIN_HIG};
    Vec2 maxSize{FLT_MAX, FLT_MAX};
    bool hasTitleBar = true;
    bool hasToolbar = false;
    bool modal = true;     // 窗口通常是模态的
    bool noResize = false; // 禁止调整大小
    bool noMove = false;   // 禁止移动
    bool noCollapse = false;
    SDL_Window* sdlWindow = nullptr;
};

/**
 * @brief 箭头组件
 */
struct Arrow
{
    static constexpr float DEFAULT_THICKNESS = 2.0F;
    static constexpr float DEFAULT_ARROW_SIZE = 10.0F;
    Vec2 startPoint{0.0F, 0.0F};
    Vec2 endPoint{100.0F, 100.0F};
    Color color{1.0F, 1.0F, 1.0F, 1.0F};
    float thickness = DEFAULT_THICKNESS;
    float arrowSize = DEFAULT_ARROW_SIZE;
};

/**
 * @brief 列表区域组件
 */
struct ListArea
{
    static constexpr float DEFAULT_ITEM_HEIGHT = 30.0F;
    std::vector<entt::entity> items;
    std::vector<int> selectedIndices;
    float itemHeight = DEFAULT_ITEM_HEIGHT;
    int selectedIndex = -1;
    bool multiSelect = false;
};

/**
 * @brief 表格组件
 */
struct TableInfo
{
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
    std::vector<float> columnWidths;
    int sortColumn = -1;
    bool resizable = true;
    bool sortable = false;
    bool sortAscending = true;
};

/**
 * @brief 线条组件
 */
struct LineInfo
{
    static constexpr float DEFAULT_THICKNESS = 2.0F;
    Vec2 startPoint{0.0F, 0.0F};
    Vec2 endPoint{100.0F, 0.0F};
    Vec4 color{1.0F, 1.0F, 1.0F, 1.0F};
    float thickness = DEFAULT_THICKNESS;
};

// ===================== 渲染与状态组件 =====================

/**
 * @brief 标题组件 (通常用于 Window/Dialog)
 */
struct Title
{
    std::string text;
};

/**
 * @brief 可选中/目标化组件
 */
struct Targetable
{
    int priority = 0;
    bool selectable = false;
};

} // namespace ui::components