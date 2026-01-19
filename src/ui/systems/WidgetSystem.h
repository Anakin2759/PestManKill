/**
 * ************************************************************************
 *
 * @file WidgetSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-11 (New)
 * @version 0.1
 * @brief 控件系统 - 处理控件生命周期相关事件
 *
 * 负责将外部窗口的尺寸变化同步到ECS根实体的Size组件，并触发布局更新。

 - InvalidateRender() 手动给窗口打脏标记
 - InvalidateLayout()  手动给窗口打布局脏标记
 -
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include <utils.h>             // 包含 Registry
#include "common/Components.h" // 包含 Size
#include "common/Tags.h"       // 包含 LayoutDirtyTag
#include "interface/Isystem.h"
#include "common/Events.h"
#include <SDL3/SDL.h>
namespace ui::systems
{

class WidgetSystem : public ui::interface::EnableRegister<WidgetSystem>
{
public:
    void registerHandlersImpl() {}
    void unregisterHandlersImpl() {}

    void onCloseWindow(events::CloseWindow event)
    {
        auto& registery = utils::Registry::getInstance();
        if (!registery.valid(event.entity))
        {
            return;
        }
        // SDL_DistoryWindow(registery.get<components::Window>(event.entity).sdlWindow);

        registery.destroy(event.entity);

        auto& dispatcher = ::utils::Dispatcher::getInstance();
        dispatcher.enqueue<ui::events::QuitRequested>();
    }

    /**
     * @brief 手动标记实体及其父链需要重新布局（用于直接修改组件后）
     *
     * @example
     * auto& size = registry.get<Size>(entity);
     * size.size.x() = 100;
     * WidgetSystem::InvalidateLayout(registry, entity); // 手动标记脏
     */
    static void invalidateLayout(entt::registry& registry, entt::entity entity)
    {
        entt::entity current = entity;
        while (current != entt::null && registry.valid(current))
        {
            registry.emplace_or_replace<components::LayoutDirtyTag>(current);
            const auto* hierarchy = registry.try_get<components::Hierarchy>(current);
            current = hierarchy != nullptr ? hierarchy->parent : entt::null;
        }
    }

    static void invalidateRender(entt::registry& registry, entt::entity entity)
    {
        // 目前仅标记自身需要重新渲染
        if (registry.valid(entity))
        {
            registry.emplace_or_replace<components::RenderDirtyTag>(entity);
        }
    }

    /**
     * @brief 按照父子层级销毁控件实体及其子实体
     */
    static void destroyWidget(entt::entity entity) {};

    static void setSize() {}
};

} // namespace ui::systems