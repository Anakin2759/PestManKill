/**
 * ************************************************************************
 *
 * @file ActionSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-28 (Updated)
 * @version 0.2
 * @brief 控件动作系统 - 处理抽象交互事件
 *
 * 职责：
 * - 处理高级抽象事件：点击(Click)、悬停(Hover)、长按(LongPress)等
 * - 触发组件上注册的回调函数
 *
 * 设计原则：
 * - 基于ECS事件驱动
 * - 易于扩展新的抽象动作类型
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <entt/entt.hpp>
#include "../common/Events.hpp"
#include "../singleton/Dispatcher.hpp"
#include "../singleton/Registry.hpp"
#include "../singleton/Logger.hpp"
#include "../interface/Isystem.hpp"
#include "../common/Components.hpp"
#include "../common/GlobalContext.hpp"
namespace ui::systems
{
class ActionSystem : public ui::interface::EnableRegister<ActionSystem>
{
public:
    void registerHandlersImpl()
    {
        // 只监听抽象事件
        Dispatcher::Sink<ui::events::ClickEvent>().connect<&ActionSystem::onClickEvent>(*this);
        Dispatcher::Sink<ui::events::HoverEvent>().connect<&ActionSystem::onHoverEvent>(*this);
        Dispatcher::Sink<ui::events::UnhoverEvent>().connect<&ActionSystem::onUnhoverEvent>(*this);
        Dispatcher::Sink<ui::events::QueuedTask>().connect<&ActionSystem::onQueuedTask>(*this);
    }

    void unregisterHandlersImpl()
    {
        Dispatcher::Sink<ui::events::ClickEvent>().disconnect<&ActionSystem::onClickEvent>(*this);
        Dispatcher::Sink<ui::events::HoverEvent>().disconnect<&ActionSystem::onHoverEvent>(*this);
        Dispatcher::Sink<ui::events::UnhoverEvent>().disconnect<&ActionSystem::onUnhoverEvent>(*this);
        Dispatcher::Sink<ui::events::QueuedTask>().disconnect<&ActionSystem::onQueuedTask>(*this);
    }

private:
    /**
     * @brief 处理点击事件
     * @param event 点击事件数据
     */
    void onClickEvent(const ui::events::ClickEvent& event)
    {
        if (!Registry::Valid(event.entity)) return;

        auto* clickable = Registry::TryGet<ui::components::Clickable>(event.entity);
        if (clickable != nullptr && clickable->enabled == policies::Feature::Enabled && clickable->onClick)
        {
            Logger::info("Entity {} clicked", static_cast<uint32_t>(event.entity));
            clickable->onClick();
        }
    }

    /**
     * @brief 处理悬浮事件
     * @param event 悬浮事件数据
     */
    void onHoverEvent(const ui::events::HoverEvent& event)
    {
        if (!Registry::Valid(event.entity)) return;

        auto* hoverable = Registry::TryGet<ui::components::Hoverable>(event.entity);
        if (hoverable != nullptr && hoverable->enabled == policies::Feature::Enabled && hoverable->onHover)
        {
            hoverable->onHover();
        }
    }

    /**
     * @brief 处理取消悬浮事件
     * @param event 取消悬浮事件数据
     */
    void onUnhoverEvent(const ui::events::UnhoverEvent& event)
    {
        if (!Registry::Valid(event.entity)) return;

        auto* hoverable = Registry::TryGet<ui::components::Hoverable>(event.entity);
        if (hoverable != nullptr && hoverable->enabled == policies::Feature::Enabled && hoverable->onUnhover)
        {
            hoverable->onUnhover();
        }
    }

    void onQueuedTask(ui::events::QueuedTask& event)
    {
        auto& frameContext = Registry::ctx().get<globalContext::FrameContext>();
        event.remainingMs =
            frameContext.intervalMs < event.remainingMs ? event.remainingMs - frameContext.intervalMs : 0;

        if (event.remainingMs == 0 && event.frameSlot != frameContext.frameSlot)
        {
            event.func();
            if (event.singleShoot)
            {
                return;
            }
            event.remainingMs = event.intervalMs;
        }

        event.frameSlot = frameContext.frameSlot;
        if (!event.quitAfterExecute)
        {
            Dispatcher::Enqueue<ui::events::QueuedTask>(std::move(event));
        }
    }
};
} // namespace ui::systems