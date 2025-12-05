/**
 * ************************************************************************
 *
 * @file UIEvents.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief UI ECS 事件定义
 *
 * UI系统中使用的所有事件类型
 * 通过entt::dispatcher进行事件分发
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include <imgui.h>
#include <string>

namespace ui::events
{

/**
 * @brief 渲染请求事件
 */
struct RenderRequest
{
    entt::entity entity;
    ImVec2 position;
    ImVec2 size;
};

/**
 * @brief 按钮点击事件
 */
struct ButtonClicked
{
    entt::entity entity;
};

/**
 * @brief 文本改变事件
 */
struct TextChanged
{
    entt::entity entity;
    std::string newText;
};

/**
 * @brief 选择改变事件
 */
struct SelectionChanged
{
    entt::entity entity;
    int selectedIndex;
};

/**
 * @brief 布局更新事件
 */
struct LayoutUpdate
{
    entt::entity entity;
};

/**
 * @brief 动画完成事件
 */
struct AnimationComplete
{
    entt::entity entity;
};

/**
 * @brief UI元素创建事件
 */
struct WidgetCreated
{
    entt::entity entity;
};

/**
 * @brief UI元素销毁事件
 */
struct WidgetDestroyed
{
    entt::entity entity;
};

/**
 * @brief 可见性改变事件
 */
struct VisibilityChanged
{
    entt::entity entity;
    bool visible;
};

} // namespace ui::events
