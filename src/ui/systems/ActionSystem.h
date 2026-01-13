/**
 * ************************************************************************
 *
 * @file ActionSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-05
 * @version 0.1
 * @brief 控件动作系统 - 处理控件动作事件

 *  处理点击 悬浮 等动作事件
    触发对应的回调函数
    基于ECS事件驱动设计
    易于扩展更多动作类型
    保持系统职责单一，专注于动作处理
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <entt/entt.hpp>
#include <utils.h>
#include "imgui_internal.h"
#include "src/ui/common/Events.h"
#include "src/utils/Dispatcher.h"
#include "interface/Isystem.h"
#include "src/ui/common/Components.h"
namespace ui::systems
{
class ActionSystem : public ui::interface::EnableRegister<ActionSystem>
{
public:
    void registerHandlersImpl()
    {
        auto& dispatcher = utils::Dispatcher::getInstance();
        dispatcher.sink<ui::events::ClickEvent>().connect<&ActionSystem::onClickEvent>(*this);
        dispatcher.sink<ui::events::HoverEvent>().connect<&ActionSystem::onHoverEvent>(*this);
        dispatcher.sink<ui::events::UnhoverEvent>().connect<&ActionSystem::onUnhoverEvent>(*this);
        dispatcher.sink<ui::events::MousePressEvent>().connect<&ActionSystem::onPressEvent>(*this);
        dispatcher.sink<ui::events::MouseReleaseEvent>().connect<&ActionSystem::onReleaseEvent>(*this);
    }
    void unregisterHandlersImpl()
    {
        auto& dispatcher = utils::Dispatcher::getInstance();
        dispatcher.sink<ui::events::ClickEvent>().disconnect<&ActionSystem::onClickEvent>(*this);
        dispatcher.sink<ui::events::HoverEvent>().disconnect<&ActionSystem::onHoverEvent>(*this);
        dispatcher.sink<ui::events::UnhoverEvent>().disconnect<&ActionSystem::onUnhoverEvent>(*this);
        dispatcher.sink<ui::events::MousePressEvent>().disconnect<&ActionSystem::onPressEvent>(*this);
        dispatcher.sink<ui::events::MouseReleaseEvent>().disconnect<&ActionSystem::onReleaseEvent>(*this);
    }

private:
    /**
     * @brief 处理点击事件
     * @param event 点击事件数据
     */
    void onClickEvent(const ui::events::ClickEvent& event)
    {
        auto& registry = utils::Registry::getInstance();
        if (!registry.valid(event.entity)) return;

        auto* clickable = registry.try_get<ui::components::Clickable>(event.entity);
        if (clickable && clickable->enabled && clickable->onClick)
        {
            LOG_INFO("Entity {} clicked", static_cast<uint32_t>(event.entity));
            clickable->onClick();
        }
    }

    /**
     * @brief 处理悬浮事件
     * @param event 悬浮事件数据
     */
    void onHoverEvent(const ui::events::HoverEvent& event)
    {
        auto& registry = utils::Registry::getInstance();
        if (!registry.valid(event.entity)) return;

        auto* hoverable = registry.try_get<ui::components::Hoverable>(event.entity);
        if (hoverable && hoverable->enabled && hoverable->onHover)
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
        auto& registry = utils::Registry::getInstance();
        if (!registry.valid(event.entity)) return;

        auto* hoverable = registry.try_get<ui::components::Hoverable>(event.entity);
        if (hoverable && hoverable->enabled && hoverable->onUnhover)
        {
            hoverable->onUnhover();
        }
    }

    /**
     * @brief 处理鼠标按下事件
     * @param event 按下事件数据
     */
    void onPressEvent(const ui::events::MousePressEvent& event)
    {
        auto& registry = utils::Registry::getInstance();
        if (!registry.valid(event.entity)) return;

        auto* pressable = registry.try_get<ui::components::Pressable>(event.entity);
        if (pressable && pressable->enabled && pressable->onPress)
        {
            pressable->onPress();
        }
    }

    /**
     * @brief 处理鼠标松开事件
     * @param event 松开事件数据
     */
    void onReleaseEvent(const ui::events::MouseReleaseEvent& event)
    {
        auto& registry = utils::Registry::getInstance();
        if (!registry.valid(event.entity)) return;

        auto* pressable = registry.try_get<ui::components::Pressable>(event.entity);
        if (pressable && pressable->enabled && pressable->onRelease)
        {
            pressable->onRelease();
        }
    }
};
} // namespace ui::systems