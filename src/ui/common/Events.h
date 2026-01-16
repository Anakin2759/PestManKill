/**
 * ************************************************************************
 *
 * @file Events.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-11 (Optimized)
 * @version 0.2
 * @brief UI ECS 事件定义
 *
 * 事件分为两类：
 * 1. 紧急事件 [IMMEDIATE] - 使用 dispatcher.trigger() 立即执行
 *    适用场景：需要即时响应的用户交互、渲染指令、退出请求等
 *
 * 2. 缓冲区事件 [BUFFERED] - 使用 dispatcher.enqueue() 加入队列
 *    适用场景：可以延迟到下一帧处理的事件，如输入事件、状态变更等
 *    在事件循环每帧开始时通过 dispatcher.update() 统一处理
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
 * @brief 在 Application 完成底层初始化 (SDL/ImGui/ECS根实体) 后触发
 * [BUFFERED] 使用 enqueue
 */
struct ApplicationReadyEvent
{
    entt::entity rootEntity;
};

/**
 * @brief 图形上下文设置事件
 * [BUFFERED] 使用 enqueue - 在下一帧开始时统一处理
 */
struct WindowGraphicsContextSetEvent
{
    entt::entity entity;
};

struct WindowGraphicsContextUnsetEvent
{
    entt::entity entity;
};

// =====================================================================
// B. 通用 UI 结构 / 动画 / 渲染事件 (由 Systems 触发)
// =====================================================================

/**
 * @brief ECS 实体创建完成事件
 * [BUFFERED] 使用 enqueue
 */
struct EntityCreated
{
    entt::entity entity;
};

/**
 * @brief ECS 实体销毁事件
 * [BUFFERED] 使用 enqueue
 */
struct EntityDestroyed
{
    entt::entity entity;
};

/**
 * @brief 布局更新事件 - 触发 LayoutSystem 重新计算布局
 * [BUFFERED] 使用 enqueue
 */
struct LayoutRecalculate
{
    entt::entity entity;
};

/**
 * @brief 动画完成事件
 * [IMMEDIATE] 使用 trigger - 需要立即响应以触发后续逻辑
 */
struct AnimationComplete
{
    entt::entity entity;
};

/**
 * @brief 可见性改变事件
 * [BUFFERED] 使用 enqueue
 */
struct VisibilityChanged
{
    entt::entity entity;
    bool visible;
};

// =====================================================================
// C. 通用 UI 交互事件 (由 InteractionSystem 触发)
//    交互事件为 [IMMEDIATE] 类型，使用 trigger 立即响应用户操作
// =====================================================================

/**
 * @brief SDL 原始事件
 * [BUFFERED] 使用 enqueue - 由任务调度器周期性触发
 */
struct SDLEvent
{
    SDL_Event event;
};

/**
 * @brief 请求退出事件
 * [IMMEDIATE] 使用 trigger - 需立即停止事件循环
 */
struct QuitRequested
{
};

/**
 * @brief 窗口尺寸变化事件
 * [IMMEDIATE] 使用 trigger
 */
struct WindowResized
{
    int width;
    int height;
};

/**
 * @brief 点击事件 - 鼠标按下并在同一实体上释放
 * [IMMEDIATE] 使用 trigger
 */
struct ClickEvent
{
    entt::entity entity;
};

/**
 * @brief 取消悬浮事件
 * [IMMEDIATE] 使用 trigger
 */
struct UnhoverEvent
{
    entt::entity entity;
};

/**
 * @brief 悬浮事件
 * [IMMEDIATE] 使用 trigger
 */
struct HoverEvent
{
    entt::entity entity;
};

/**
 * @brief 鼠标按下事件
 * [IMMEDIATE] 使用 trigger
 */
struct MousePressEvent
{
    entt::entity entity;
};

/**
 * @brief 鼠标释放事件
 * [IMMEDIATE] 使用 trigger
 */
struct MouseReleaseEvent
{
    entt::entity entity;
};
/**
 * @brief 文本内容改变事件 (TextEdit/Input)
 * [BUFFERED] 使用 enqueue
 */
struct ValueChangedText
{
    entt::entity entity;
    std::string newText;
};

/**
 * @brief 选择索引改变事件 (Dropdown/List)
 * [BUFFERED] 使用 enqueue
 */
struct ValueChangedSelection
{
    entt::entity entity;
    int selectedIndex;
};

/**
 * @brief 发送处理函数到事件循环
 * [BUFFERED] 使用 enqueue - 在事件循环中执行回调
 */
struct SendHandlerToEventLoop
{
    std::move_only_function<void()> handler;
};

/**
 * @brief 通用更新事件
 * [BUFFERED] 使用 enqueue
 */
struct UpdateEvent
{
};

struct CreateWindow
{
    std::string title;
    std::string alias;
};
/**
 * @brief 关闭窗口事件
 * [IMMEDIATE] 使用 trigger
 */
struct CloseWindow
{
    entt::entity entity;
};

/**
 * @brief 渲染更新事件 - 每帧渲染时触发
 * [IMMEDIATE] 使用 trigger - 需立即执行渲染
 */
struct UpdateRendering
{
};

/**
 * @brief 布局更新事件 - 每帧渲染前触发
 * [IMMEDIATE] 使用 trigger - 需在渲染前立即完成布局
 */
struct UpdateLayout
{
};

struct QuequedTask
{
    std::move_only_function<void()> func;
};

} // namespace ui::events