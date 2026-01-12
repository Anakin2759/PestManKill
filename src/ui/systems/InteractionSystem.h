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

#include <utility>
#include <SDL3/SDL.h>
#include <imgui_impl_sdl3.h>
#include "components/Events.h"
#include "src/utils/Registry.h"    // 包含 Registry
#include "src/utils/Dispatcher.h"  // 包含 Dispatcher
#include "components/Components.h" // 包含 Position, Size, Clickable, ButtonState, Hierarchy
#include "components/Tags.h"       // 包含 HoveredTag, ActiveTag, DisabledTag, LayoutDirtyTag
#include "src/ui/interface/Isystem.h"
namespace ui::systems
{

class InteractionSystem : public ui::interface::EnableRegister<InteractionSystem>
{
public:
    void registerHandlersImpl()
    {
        auto& dispatcher = utils::Dispatcher::getInstance();
        dispatcher.sink<ui::events::SDLEvent>().connect<&InteractionSystem::onSDLEvent>(*this);
    }

    void unregisterHandlersImpl() {}

private:
    /**
     * @brief 处理每一帧输入事件和交互状态更新
     * @param mousePressed 本帧是否有鼠标按下事件（来自 SDL）
     * @param mouseReleased 本帧是否有鼠标释放事件（来自 SDL）
     */
    void getInput(bool mousePressed, bool mouseReleased) noexcept
    {
        auto& registry = ::utils::Registry::getInstance();
        auto& dispatcher = ::utils::Dispatcher::getInstance();

        // 获取鼠标位置（SDL API 直接获取，不依赖 ImGui IO 帧更新）
        float mouseX = 0.0F, mouseY = 0.0F;
        SDL_GetMouseState(&mouseX, &mouseY);
        ImVec2 mousePos(mouseX, mouseY);

        // 获取 Z-Order 排序的可交互实体
        auto interactables = getZOrderedInteractables(registry);

        entt::entity hoveredEntity = entt::null;

        // 从前到后进行碰撞测试
        for (auto entity : interactables)
        {
            [[maybe_unused]] const auto& pos = registry.get<const components::Position>(entity);
            const auto& size = registry.get<const components::Size>(entity);

            ImVec2 absPos = getAbsolutePosition(registry, entity);

            if (isPointInRect(mousePos, absPos, size.size))
            {
                hoveredEntity = entity;
                break; // 找到最前面的命中实体，停止检测
            }
        }

        // 更新 Hover 状态并触发事件
        if (hoveredEntity != m_hoveredEntity)
        {
            // 触发 UnhoverEvent（如果之前有悬浮的实体）
            if (m_hoveredEntity != entt::null && registry.valid(m_hoveredEntity))
            {
                registry.remove<components::HoveredTag>(m_hoveredEntity);
                dispatcher.enqueue<events::UnhoverEvent>(events::UnhoverEvent{m_hoveredEntity});
            }

            // 触发 HoverEvent（如果现在有新的悬浮实体）
            if (hoveredEntity != entt::null)
            {
                registry.emplace_or_replace<components::HoveredTag>(hoveredEntity);
                dispatcher.enqueue<events::HoverEvent>(events::HoverEvent{hoveredEntity});
            }

            m_hoveredEntity = hoveredEntity;
        }

        // 处理鼠标按下（使用 SDL 事件驱动的状态）
        if (mousePressed && hoveredEntity != entt::null)
        {
            if (m_activeEntity == entt::null)
            {
                m_activeEntity = hoveredEntity;
                registry.emplace_or_replace<components::ActiveTag>(m_activeEntity);

                // 触发按下事件
                if (registry.any_of<components::Pressable>(m_activeEntity))
                {
                    dispatcher.enqueue<events::MousePressEvent>(events::MousePressEvent{m_activeEntity});
                }
            }
        }

        // 处理鼠标释放（使用 SDL 事件驱动的状态）
        if (mouseReleased && m_activeEntity != entt::null)
        {
            // 触发松开事件（在移除 ActiveTag 之前）
            if (registry.any_of<components::Pressable>(m_activeEntity))
            {
                dispatcher.enqueue<events::MouseReleaseEvent>(events::MouseReleaseEvent{m_activeEntity});
            }

            registry.remove<components::ActiveTag>(m_activeEntity);

            // 如果释放时仍然在同一实体上，触发点击事件
            if (m_activeEntity == hoveredEntity)
            {
                if (registry.try_get<components::Clickable>(m_activeEntity) != nullptr)
                {
                    dispatcher.enqueue<events::ClickEvent>(events::ClickEvent{m_activeEntity});
                }
            }

            m_activeEntity = entt::null;
        }
    }

    /**
     * @brief 处理 SDL每tick事件
     *
     * - 负责 SDL_PollEvent 事件
     * - 将事件转发给 ImGui 后端 (ImGui_ImplSDL3_ProcessEvent)
     * - 识别 Quit / Window Resized，并通过回调交由上层处理
     * - 直接从 SDL 事件中追踪鼠标状态（避免依赖 ImGui IO 时序问题）
     */
    void onSDLEvent(ui::events::SDLEvent& sdlEvent)
    {
        auto& dispatcher = ::utils::Dispatcher::getInstance();

        auto& event = sdlEvent.event;
        bool mousePressed = false;
        bool mouseReleased = false;

        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    dispatcher.enqueue<ui::events::QuitRequested>();
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    // 可以触发 WindowResized 事件
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        mousePressed = true;
                        m_mouseDown = true;
                    }
                    break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        mouseReleased = true;
                        m_mouseDown = false;
                    }
                    break;
                default:
                    break;
            }
        }

        // 处理交互检测（在事件处理完成后）
        getInput(mousePressed, mouseReleased);
    }

    entt::entity m_activeEntity = entt::null;  // 当前处于 Active (鼠标按下) 状态的实体
    entt::entity m_hoveredEntity = entt::null; // 当前悬浮的实体
    bool m_mouseDown = false;                  // 鼠标按下状态（基于 SDL 事件追踪）

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
     *
     * @note 对于 Window/Dialog 容器的子元素，需要加上 ImGui 窗口的内容区偏移
     *       以与 RenderSystem 中的渲染位置保持一致。
     */
    ImVec2 getAbsolutePosition(entt::registry& registry, entt::entity entity)
    {
        // 构建从当前实体到根的路径
        std::vector<entt::entity> path;
        entt::entity current = entity;
        while (current != entt::null && registry.valid(current))
        {
            path.push_back(current);
            const auto* hierarchy = registry.try_get<components::Hierarchy>(current);
            current = hierarchy ? hierarchy->parent : entt::null;
        }

        // 从根到当前实体正向遍历，模拟 RenderSystem 的递归逻辑
        ImVec2 pos(0.0f, 0.0f);
        for (auto it = path.rbegin(); it != path.rend(); ++it)
        {
            entt::entity e = *it;
            const auto* posComp = registry.try_get<components::Position>(e);
            if (posComp)
            {
                pos.x += posComp->value.x;
                pos.y += posComp->value.y;
            }

            // 检查是否是 Window/Dialog 容器
            // 如果是，需要获取 ImGui 窗口的内容区偏移（与 RenderSystem 保持一致）
            if (registry.any_of<components::WindowTag, components::DialogTag>(e))
            {
                const auto* windowComp = registry.try_get<components::Window>(e);
                if (windowComp)
                {
                    // 获取 ImGui 窗口状态以计算内容区偏移
                    // ImGui::FindWindowByName 返回 ImGuiWindow*，但这是内部 API
                    // 改用 SetNextWindowPos/Begin 配合 GetCursorScreenPos 的差值
                    // 但由于我们在事件处理中而非渲染帧中，无法直接调用 ImGui::Begin
                    // 因此使用预估值：标题栏高度约为 ImGui::GetFrameHeight()，无标题栏则为 0
                    // 加上 ImGui 窗口的默认 Padding

                    float titleBarOffset = windowComp->hasTitleBar ? ImGui::GetFrameHeight() : 0.0f;
                    ImVec2 windowPadding = ImGui::GetStyle().WindowPadding;

                    // 从当前实体到根已经累加了窗口的 Position
                    // 现在需要为下一层（子元素）添加内容区偏移
                    // 但我们是从根到叶遍历，所以在处理完窗口后加上偏移
                    if (it + 1 != path.rend()) // 如果不是最终目标实体
                    {
                        pos.x += windowPadding.x;
                        pos.y += titleBarOffset + windowPadding.y;
                    }
                }
            }
        }
        return pos;
    }

    /**
     * @brief 获取按 Z-Order 从前到后排序的可交互实体列表
     * @note 实际项目中，UI 实体应该根据 Z-Index Component 或渲染顺序预排序。
     * 此处为简化，仅按 Hierarchy 深度排序（深度越大越靠前）
     */
    static std::vector<entt::entity> getZOrderedInteractables(entt::registry& registry)
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