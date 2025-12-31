/**
 * ************************************************************************
 *
 * @file InteractionSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-11 (Updated)
 * @version 0.2
 * @brief 交互处理系统，事件驱动
 *
 * 将原始输入事件映射到UI实体的交互组件上，并更新 Hover/Active/Dirty 等 ECS 状态。
 * 负责处理点击、悬停等交互逻辑，触发相应的UI事件和通知后台。
    从SDL或ImGui获取鼠标位置和按键状态。
    遍历所有可交互实体，执行点碰撞测试 (Hit Test)。
    更新实体的 HoveredTag 和 ActiveTag 组件。
    处理点击事件，触发 ButtonClickedEvent 等UI事件。
    管理交互状态，确保正确的状态转换和事件触发。
    优化交互检测顺序，支持Z-Order排序。
    易于扩展以支持更多交互类型和复杂逻辑。
 *
 * ************************************************************************
 */
#pragma once
#include <entt/entt.hpp>
#include <algorithm>
#include <concepts>
#include <functional>
#include <utility>
#include <SDL3/SDL.h>
#include <imgui_impl_sdl3.h>
#include "components/Events.h"
#include "src/utils/Registry.h"    // 包含 Registry
#include "src/utils/Dispatcher.h"  // 包含 Dispatcher
#include "components/Components.h" // 包含 Position, Size, Clickable, ButtonState, Hierarchy
#include "components/Tags.h"       // 包含 HoveredTag, ActiveTag, DisabledTag, LayoutDirtyTag

namespace ui::systems
{

class InteractionSystem : public ui::interface::EnableRegister<InteractionSystem>
{
public:
    void registerHandlersImpl() {}

    void unregisterHandlersImpl() {}

private:
    /**
     * @brief 处理每一帧输入事件和交互状态更新
     */
    void getInput() noexcept
    {
        auto& registry = ::utils::Registry::getInstance();
        auto& dispatcher = ::utils::Dispatcher::getInstance();

        // 获取鼠标状态
        ImGuiIO& io = ImGui::GetIO();
        ImVec2 mousePos = io.MousePos;
        bool mouseDown = io.MouseDown[0];
        bool mouseReleased = io.MouseReleased[0];

        // 如果 ImGui 想要捕获鼠标（例如在 ImGui 窗口上），跳过 UI 交互检测
        if (io.WantCaptureMouse)
        {
            return;
        }

        // 清除所有实体的 HoveredTag
        auto hoveredView = registry.view<components::HoveredTag>();
        for (auto entity : hoveredView)
        {
            registry.remove<components::HoveredTag>(entity);
        }

        // 获取 Z-Order 排序的可交互实体
        auto interactables = getZOrderedInteractables(registry);

        entt::entity hoveredEntity = entt::null;

        // 从前到后进行碰撞测试
        for (auto entity : interactables)
        {
            const auto& pos = registry.get<const components::Position>(entity);
            const auto& size = registry.get<const components::Size>(entity);

            ImVec2 absPos = getAbsolutePosition(registry, entity);

            if (isPointInRect(mousePos, absPos, size.size))
            {
                hoveredEntity = entity;
                break; // 找到最前面的命中实体，停止检测
            }
        }

        // 更新 Hover 状态
        if (hoveredEntity != entt::null)
        {
            registry.emplace_or_replace<components::HoveredTag>(hoveredEntity);
        }

        // 处理鼠标按下
        if (mouseDown && hoveredEntity != entt::null)
        {
            if (m_activeEntity == entt::null)
            {
                m_activeEntity = hoveredEntity;
                registry.emplace_or_replace<components::ActiveTag>(m_activeEntity);
            }
        }

        // 处理鼠标释放
        if (mouseReleased && m_activeEntity != entt::null)
        {
            registry.remove<components::ActiveTag>(m_activeEntity);

            // 如果释放时仍然在同一实体上，触发点击事件
            if (m_activeEntity == hoveredEntity)
            {
                if (auto* clickable = registry.try_get<components::Clickable>(m_activeEntity))
                {
                    if (clickable->onClick)
                    {
                        clickable->onClick(m_activeEntity);
                    }
                    dispatcher.trigger<events::ButtonClick>(events::ButtonClick{m_activeEntity});
                }
            }

            m_activeEntity = entt::null;
        }
    }

    /**
     * @brief 处理 SDL每tick事件 不用循环
     *
     * - 负责 SDL_PollEvent 事件
     * - 将事件转发给 ImGui 后端 (ImGui_ImplSDL3_ProcessEvent)
     * - 识别 Quit / Window Resized，并通过回调交由上层处理
     */
    void processEvents()
    {
        auto& dispatcher = ::utils::Dispatcher::getInstance();

        SDL_Event event{};
        if (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT)
            {
                dispatcher.trigger<ui::events::QuitRequested>();
            }

            if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                dispatcher.trigger<ui::events::WindowResized>(
                    ui::events::WindowResized{event.window.data1, event.window.data2});
            }

            // 保持与原逻辑一致：上层自定义事件处理在 Quit/Resize 之后执行。
            dispatcher.trigger<ui::events::SDLEvent>(ui::events::SDLEvent{event});
        }
    }

    entt::entity m_activeEntity = entt::null; // 当前处于 Active (鼠标按下) 状态的实体

    /**
     * @brief 执行点碰撞测试 (Hit Test)
     * @param point 鼠标绝对位置
     * @param pos 实体绝对位置
     * @param size 实体尺寸
     * @return bool 是否命中
     */
    bool isPointInRect(const ImVec2& point, const ImVec2& pos, const ImVec2& size) const
    {
        return point.x >= pos.x && point.x < (pos.x + size.x) && point.y >= pos.y && point.y < (pos.y + size.y);
    }

    /**
     * @brief 计算实体的绝对位置
     * @param registry 注册表
     * @param entity 当前实体
     * @return ImVec2 实体的绝对位置
     */
    ImVec2 getAbsolutePosition(entt::registry& registry, entt::entity entity)
    {
        ImVec2 pos(0.0f, 0.0f);
        entt::entity current = entity;

        // 遍历层级，累加相对位置
        while (current != entt::null && registry.valid(current))
        {
            const auto* posComp = registry.try_get<components::Position>(current);
            if (posComp)
            {
                pos.x += posComp->value.x;
                pos.y += posComp->value.y;
            }
            const auto* hierarchy = registry.try_get<components::Hierarchy>(current);
            current = hierarchy ? hierarchy->parent : entt::null;
        }
        return pos;
    }

    /**
     * @brief 获取按 Z-Order 从前到后排序的可交互实体列表
     * @note 实际项目中，UI 实体应该根据 Z-Index Component 或渲染顺序预排序。
     * 此处为简化，仅按 Hierarchy 深度排序（深度越大越靠前）
     */
    std::vector<entt::entity> getZOrderedInteractables(entt::registry& registry)
    {
        std::vector<std::pair<int, entt::entity>> interactables;
        auto view = registry.view<const components::Position, const components::Size, const components::Clickable>();

        for (auto entity : view)
        {
            if (registry.any_of<components::DisabledTag>(entity)) continue; // 忽略禁用的实体

            // 简单深度排序：层级越深（子元素），Z-Order 越高
            int depth = 0;
            entt::entity current = entity;
            while (current != entt::null)
            {
                const auto* hierarchy = registry.try_get<components::Hierarchy>(current);
                current = hierarchy ? hierarchy->parent : entt::null;
                depth++;
            }
            interactables.emplace_back(depth, entity);
        }

        // 排序：深度越大（越靠近前端）的排在前面
        std::sort(
            interactables.begin(), interactables.end(), [](const auto& a, const auto& b) { return a.first > b.first; });

        std::vector<entt::entity> result;
        for (const auto& pair : interactables)
        {
            result.push_back(pair.second);
        }
        return result;
    }
};

} // namespace ui::systems