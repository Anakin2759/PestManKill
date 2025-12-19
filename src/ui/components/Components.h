/**
 * ************************************************************************
 *
 * @file Components.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @brief UI ECS 组件定义 (完整且优化版)
 *
 * 遵循ECS模式：纯数据结构，无行为逻辑，只包含属性
 * 确保组件的纯粹性和独立性
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
#include "UIDefine.h" // 假设 UIDefine.h 中包含了所有必要的枚举和常量

namespace ui::components
{

// ===================== 基础尺寸与位置 =====================

/**
 * @brief 尺寸约束组件
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
 * @brief UI元素的相对位置组件
 */
struct Position
{
    float x = 0.0F;
    float y = 0.0F;
};

/**
 * @brief 边距组件 (外边距)
 */
struct Margin
{
    ImVec4 values{0, 0, 0, 0}; // Top, Right, Bottom, Left
};
/**
 * @brief 内边距组件
 * 定义元素内容与其边框之间的空间。
 */
struct Padding
{
    // 对应 ImVec4(Top, Right, Bottom, Left)
    ImVec4 values{0, 0, 0, 0};
};
/**
 * @brief 背景绘制组件 (包含圆角)
 */
struct Background
{
    ImVec4 color{0.0F, 0.0F, 0.0F, 0.0F};
    float borderRadius = 0.0F; // 圆角半径
    bool enabled = false;
};

/**
 * @brief 边框组件
 */
struct Border
{
    ImVec4 color{1.0F, 1.0F, 1.0F, 1.0F};
    float thickness = 1.0F;
    float borderRadius = 0.0F; // 圆角半径
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
    ImVec2 scrollOffset{0.0f, 0.0f}; // 当前滚动位置
    ImVec2 contentSize{0.0f, 0.0f};  // 内容区域大小
    bool horizontalScroll = false;
    bool verticalScroll = true;
    float scrollSpeed = 10.0f;
    bool showScrollbars = true; // 原始属性保留
};

// ===================== 布局组件 =====================

/**
 * @brief 布局容器配置组件
 */
struct LayoutInfo
{
    LayoutDirection direction = LayoutDirection::HORIZONTAL;
    float spacing = 5.0F; // 元素间距
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
    ImVec4 color{1.0F, 1.0F, 1.0F, 1.0F};
    float fontSize = 0.0F; // 0 表示使用 ImGui 默认字体大小
    Alignment alignment = Alignment::NONE;
    bool wordWrap = false;
    float wrapWidth = 0.0F;
};

/**
 * @brief 文本编辑框数据组件
 */
struct TextEdit
{
    std::string buffer; // 存储输入文本的缓冲区
    std::string placeholder;
    ImVec4 textColor{1.0F, 1.0F, 1.0F, 1.0F};
    bool multiline = false;
    bool readOnly = false;
    bool password = false;
    size_t maxLength = 256;
};

// ===================== 图像组件 (完整定义) =====================

/**
 * @brief 图像组件
 */
struct Image
{
    void* textureId = nullptr;                  // 纹理句柄 (例如 SDL_Texture* 或 OpenGL ID)
    ImVec2 uvMin{0.0F, 0.0F};                   // UV 最小坐标
    ImVec2 uvMax{1.0F, 1.0F};                   // UV 最大坐标
    ImVec4 tintColor{1.0F, 1.0F, 1.0F, 1.0F};   // 颜色叠加
    ImVec4 borderColor{0.0F, 0.0F, 0.0F, 0.0F}; // 边框颜色 (原始属性保留)
    bool maintainAspectRatio = true;            // 是否保持宽高比 (原始属性保留)
};

// ===================== 交互与状态 =====================

/**
 * @brief 可点击组件
 */
struct Clickable
{
    bool enabled = true;
    std::function<void(entt::entity)> onClick{};
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
    ButtonVisualState visual = ButtonVisualState::Idle;
    bool triggered = false; // 本帧是否触发过动作
};

// ===================== 动画组件 (精简) =====================

/**
 * @brief 动画时间状态
 */
struct AnimationTime
{
    float duration = 1.0F;
    float elapsed = 0.0F;
    EasingType easing = EasingType::Linear;
    PlayMode mode = PlayMode::ONCE;
};

/**
 * @brief 位置动画目标
 */
struct AnimationPosition
{
    ImVec2 from;
    ImVec2 to;
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
    std::string title;
    bool hasTitleBar = true;
    bool hasToolbar = false;
    bool modal = true;
    bool noResize = false;
    bool noMove = false;
    bool noCollapse = false;
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
    bool selectable = false;
    int priority = 0;
};

} // namespace ui::components