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
 * 遵循ECS模式：纯数据结构，无行为逻辑
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

namespace ui::components
{

// ===================== 基础组件 =====================

/**
 * @brief UI元素的位置组件
 */
struct Position
{
    float x = 0.0F;
    float y = 0.0F;
    bool useCustomPosition = false;
};

/**
 * @brief UI元素的尺寸组件
 */
struct Size
{
    float width = 0.0F;
    float height = 0.0F;
    bool useFixedSize = false;

    float minWidth = 0.0F;
    float minHeight = 0.0F;
    float maxWidth = FLT_MAX;
    float maxHeight = FLT_MAX;
};

/**
 * @brief 可见性组件
 */
struct Visibility
{
    bool visible = true;
    float alpha = 1.0F;
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
 * @brief 层级关系组件 - 父子关系
 */
struct Hierarchy
{
    entt::entity parent = entt::null;
    std::vector<entt::entity> children;
};

/**
 * @brief 渲染状态组件 - 防止重入
 */
struct RenderState
{
    bool isRendering = false;
};

/**
 * @brief 可选中组件 (Checkable)
 */
struct Checkable
{
    bool checkable = false;
    bool checked = false;
};

// ===================== 布局组件 =====================

/**
 * @brief 布局方向枚举
 */
enum class LayoutDirection : uint8_t
{
    HORIZONTAL,
    VERTICAL
};

/**
 * @brief 对齐方式枚举
 */
enum class Alignment : uint8_t
{
    LEFT = 0x01,
    RIGHT = 0x02,
    TOP = 0x04,
    BOTTOM = 0x08,
    H_CENTER = 0x10,
    V_CENTER = 0x20,
    CENTER = H_CENTER | V_CENTER
};

/**
 * @brief 布局项数据
 */
struct LayoutItem
{
    entt::entity widget = entt::null;
    int stretchFactor = 0;
    Alignment alignment = Alignment::LEFT;
};

/**
 * @brief 布局容器组件
 */
struct Layout
{
    LayoutDirection direction = LayoutDirection::HORIZONTAL;
    std::vector<LayoutItem> items;
    float spacing = 5.0F;
    ImVec4 margins{0, 0, 0, 0}; // left, top, right, bottom
};

/**
 * @brief 间隔器组件
 */
struct Spacer
{
    int stretchFactor = 1;
    bool isHorizontal = true;
};

// ===================== 交互组件 =====================

/**
 * @brief 按钮组件
 */
struct Button
{
    std::string text;
    std::string uniqueId;
    std::string tooltip;
    std::function<void()> onClick;

    bool enabled = true;
    bool useCustomColor = false;

    ImVec4 buttonColor{0.26F, 0.59F, 0.98F, 0.40F};
    ImVec4 hoverColor{0.26F, 0.59F, 0.98F, 1.00F};
    ImVec4 activeColor{0.06F, 0.53F, 0.98F, 1.00F};
};

/**
 * @brief 文本标签组件
 */
struct Label
{
    std::string text;
    ImVec4 textColor{1.0F, 1.0F, 1.0F, 1.0F};
    bool wordWrap = false;
    float wrapWidth = 0.0F;
};

/**
 * @brief 文本编辑框组件
 */
struct TextEdit
{
    std::string text;
    std::string placeholder;
    std::string uniqueId;
    std::function<void(const std::string&)> onTextChanged;

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
 * @brief 对话框组件
 */
struct Dialog
{
    std::string title;
    bool isOpen = false;
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
struct Table
{
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
    std::vector<float> columnWidths;
    bool resizable = true;
    bool sortable = false;
    int sortColumn = -1;
    bool sortAscending = true;
};

// ===================== 动画组件 =====================

/**
 * @brief 动画状态组件
 */
struct Animation
{
    bool active = false;
    float duration = 1.0F;
    float elapsed = 0.0F;
    std::function<void(float)> updateCallback;
};

/**
 * @brief 位置动画组件
 */
struct PositionAnimation
{
    ImVec2 startPos{0.0F, 0.0F};
    ImVec2 endPos{0.0F, 0.0F};
    float progress = 0.0F;
};

/**
 * @brief 透明度动画组件
 */
struct AlphaAnimation
{
    float startAlpha = 1.0F;
    float endAlpha = 0.0F;
    float progress = 0.0F;
};

// ===================== 标签组件 =====================

/**
 * @brief UI元素类型标签
 */
struct WidgetTag
{
};
struct LayoutTag
{
};
struct ButtonTag
{
};
struct LabelTag
{
};
struct TextEditTag
{
};
struct ImageTag
{
};
struct DialogTag
{
};
struct ArrowTag
{
};
struct ListAreaTag
{
};
struct TableTag
{
};
struct SpacerTag
{
};
struct ApplicationTag
{
};

} // namespace ui::components
