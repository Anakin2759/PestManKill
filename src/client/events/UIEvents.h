/**
 * ************************************************************************
 *
 * @file UIEvents.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-11 (Optimized)
 * @version 0.2
 * @brief UI ECS 事件定义：优化后的通用事件与游戏特定事件分离。
 *
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include <string>
#include <vector>

// 注意：ImGui.h 通常只在渲染系统和组件中使用，事件可以不依赖它。

namespace ui::events
{

// =====================================================================
// A. 核心 ECS / 生命周期事件 (由 Application/Engine 触发)
// =====================================================================

/**
 * @brief 在 Application 完成底层初始化 (SDL/ImGui/ECS根实体) 后触发。
 * 驱动 UI 结构初始化系统 (SetupSystem) 开始工作。
 */
struct ApplicationReadyEvent
{
    entt::entity rootEntity;
};

// =====================================================================
// B. 通用 UI 结构 / 动画 / 渲染事件 (由 Systems 触发)
// =====================================================================

/**
 * @brief ECS 实体创建完成事件。
 * 替代 WidgetCreated。
 */
struct EntityCreated
{
    entt::entity entity;
};

/**
 * @brief ECS 实体销毁事件。
 * 替代 WidgetDestroyed。
 */
struct EntityDestroyed
{
    entt::entity entity;
};

/**
 * @brief 布局更新事件。
 * 触发 LayoutSystem 重新计算布局。
 * 替代 LayoutUpdate。
 */
struct LayoutRecalculate
{
    entt::entity entity;
};

/**
 * @brief 动画完成事件。
 */
struct AnimationComplete
{
    entt::entity entity;
};

/**
 * @brief 可见性改变事件。
 */
struct VisibilityChanged
{
    entt::entity entity;
    bool visible;
};

// =====================================================================
// C. 通用 UI 交互事件 (由 InteractionSystem 触发)
// =====================================================================

/**
 * @brief 按钮点击事件。
 * 合并 ButtonClicked 和 ButtonClickEvent，只保留核心信息。
 */
struct ButtonClick
{
    entt::entity entity;
    // 如果需要标识，InteractionSystem 应负责查询该实体的 Name/ID 组件。
};

/**
 * @brief 文本内容改变事件 (TextEdit/Input)。
 * 替代 TextChanged。
 */
struct ValueChangedText
{
    entt::entity entity;
    std::string newText;
};

/**
 * @brief 选择索引改变事件 (Dropdown/List)。
 * 替代 SelectionChanged。
 */
struct ValueChangedSelection
{
    entt::entity entity;
    int selectedIndex;
};

// =====================================================================
// D. 游戏特定事件 (Card Game Logic)
// ---------------------------------------------------------------------
// 推荐将这些事件移动到独立的 GameEvents.h 中，以保持 UI 系统的纯净性。
// =====================================================================

/**
 * @brief 手牌选中状态改变事件。
 * 移除 cardName，系统应通过 cardEntity 查询其名称组件。
 */
struct CardSelectionChanged
{
    entt::entity cardEntity;
    bool selected;
};

/**
 * @brief 卡牌移动到处理区事件。
 */
struct CardMovedToProcessing
{
    entt::entity cardEntity;
};

/**
 * @brief 卡牌从处理区移除事件。
 */
struct CardRemovedFromProcessing
{
    entt::entity cardEntity;
};

/**
 * @brief 处理区清空前事件。
 */
struct ProcessingAreaBeforeClear
{
    std::vector<entt::entity> cardEntities; // 携带待清除的实体列表
};

/**
 * @brief 玩家明确执行使用卡牌事件。
 */
struct UseCardEvent
{
    entt::entity cardEntity; // 如果是单卡使用
};

/**
 * @brief 玩家明确执行取消操作事件。
 */
struct CancelOperationEvent
{
};

/**
 * @brief 玩家明确执行结束回合事件。
 */
struct EndTurnEvent
{
};

} // namespace ui::events