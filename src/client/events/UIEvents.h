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

/**
 * @brief 按钮点击事件（带标识）
 */
struct ButtonClickEvent
{
    entt::entity entity;
    std::string buttonId;
};

/**
 * @brief 手牌选中状态改变事件
 */
struct CardSelectionChanged
{
    entt::entity cardEntity;
    bool selected;
    std::string cardName;
};

/**
 * @brief 手牌移动到处理区事件
 */
struct CardMovedToProcessing
{
    entt::entity cardEntity;
    std::string cardName;
};

/**
 * @brief 卡牌从处理区移除事件
 */
struct CardRemovedFromProcessing
{
    entt::entity cardEntity;
    std::string cardName;
};

/**
 * @brief 处理区清空前事件
 */
struct ProcessingAreaBeforeClear
{
    std::vector<entt::entity> cardEntities;
};

/**
 * @brief 处理区清空后事件
 */
struct ProcessingAreaAfterClear
{
};

/**
 * @brief 使用卡牌事件
 */
struct UseCardEvent
{
    entt::entity cardEntity;
    std::string cardName;
};

/**
 * @brief 取消操作事件
 */
struct CancelOperationEvent
{
};

/**
 * @brief 结束回合事件
 */
struct EndTurnEvent
{
};

} // namespace ui::events
