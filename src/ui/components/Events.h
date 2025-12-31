/**
 * ************************************************************************
 *
 * @file Events.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-11 (Optimized)
 * @version 0.2
 * @brief UI ECS 事件定义：只有控件事件：显示/隐藏。
 *
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include <SDL3/SDL.h>
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

/**
 * @brief 图形上下文设置事件
 * 当 Application 创建 GraphicsContext 后发布此事件
 */
struct GraphicsContextSetEvent
{
    void* graphicsContext; // GraphicsContext* (避免循环依赖)
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
 * @brief SDL 原始事件（已拷贝一份 SDL_Event）。
 *
 * 用于让上层或其他系统监听未被 ECS 消耗的输入/窗口事件。
 */
struct SDLEvent
{
    SDL_Event event;
};

/**
 * @brief 请求退出事件（通常由 SDL_EVENT_QUIT 触发）。
 */
struct QuitRequested
{
};

/**
 * @brief 窗口尺寸变化事件（通常由 SDL_EVENT_WINDOW_RESIZED 触发）。
 */
struct WindowResized
{
    int width;
    int height;
};

/**
 * @brief 按钮点击事件。
 * 合并 ButtonClicked 和 ButtonClickEvent，只保留核心信息。
 */
struct ButtonClick
{
    entt::entity entity;
    // 如果需要标识，InteractionSystem 应负责查询该实体的 Name/ID 组件。
};

// 兼容旧命名
using ButtonClickedEvent = ButtonClick;

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








struct SendHandlerToEventLoop
{
    std::move_only_function<void()> handler;
};

} // namespace ui::events