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
namespace ui::systems
{
class ActionSystem : public ui::interface::EnableRegister<ActionSystem>
{
public:
    void registerHandlersImpl()
    {
        auto& dispatcher = utils::Dispatcher::getInstance();
        dispatcher.sink<ui::events::ClickEvent>().connect<&ActionSystem::onClickEvent>(*this);
    }
    void unregisterHandlersImpl()
    {
        auto& dispatcher = utils::Dispatcher::getInstance();
        dispatcher.sink<ui::events::ClickEvent>().disconnect<&ActionSystem::onClickEvent>(*this);
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
            clickable->onClick();
        }
    }

    void onHoverEvent(const ui::events::HoverEvent& event)
    {
        // 目前不处理悬浮事件
    }

    
};
} // namespace ui::systems