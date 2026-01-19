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
    void registerHandlersImpl()
    {
        auto& dispatcher = utils::Dispatcher::getInstance();
        dispatcher.sink<events::CloseWindow>().connect<&WidgetSystem::onCloseWindow>(*this);
    }

    void unregisterHandlersImpl() {}

    void onCloseWindow(const events::CloseWindow& event)
    {
        // 调用递归销毁
        if (utils::Registry::getInstance().valid(event.entity))
        {
            destroyWidget(event.entity);
        }

        // 检查是否还有任何窗口实体
        auto& registry = utils::Registry::getInstance();

        if (registry.view<components::Window>().empty())
        {
            // 没有窗口实体了，发出退出请求
            utils::Dispatcher::getInstance().trigger<ui::events::QuitRequested>();
        }
    }

    static void onRemoveWidget(entt::entity entity) { destroyWidget(entity); }

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
     * @brief 按照父子层级销毁控件实体及其子实体，并清理SDL资源
     */
    static void destroyWidget(entt::entity entity)
    {
        auto& registry = utils::Registry::getInstance();
        if (!registry.valid(entity)) return;

        // 1. 递归销毁子节点
        if (auto* hierarchy = registry.try_get<components::Hierarchy>(entity))
        {
            // 复制一份子节点列表，防止在销毁过程中因引用失效导致崩溃
            auto children = hierarchy->children;
            for (auto child : children)
            {
                destroyWidget(child);
            }
        }

        // 2. 如果是窗口，查找并销毁关联的 SDL_Window 资源
        if (auto* windowComp = registry.try_get<components::Window>(entity))
        {
            // 通过 WindowID 找回 SDL_Window 指针
            if (SDL_Window* sdlWindow = SDL_GetWindowFromID(windowComp->windowID))
            {
                // 通知 RenderSystem 解绑上下文（尽管 RenderSystem 可能目前未执行动作，保留逻辑完整性）
                ::utils::Dispatcher::getInstance().trigger<events::WindowGraphicsContextUnsetEvent>(
                    events::WindowGraphicsContextUnsetEvent{entity});

                SDL_DestroyWindow(sdlWindow);
            }
        }

        // 3. 最后销毁实体本身
        registry.destroy(entity);
    }

    static void setSize() {}
};

} // namespace ui::systems