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
    从SDL获取鼠标位置和按键状态。
    遍历所有可交互实体，执行点碰撞测试 (Hit Test)。
    更新实体的 HoveredTag 和 ActiveTag 组件。
    处理点击事件，触发 ButtonClickedEvent 等UI事件。
    管理交互状态，确保正确的状态转换和事件触发。
    优化交互检测顺序，支持Z-Order排序。
    易于扩展以支持更多交互类型和复杂逻辑。

    目标：
    实现高效、准确的UI交互处理，提升用户体验。
 *
 * ************************************************************************
 */
#pragma once
#include <entt/entt.hpp>
#include <algorithm>

#include <utility>
#include <SDL3/SDL.h>
#include "common/Events.h"
#include "src/utils/Registry.h"   // 包含 Registry
#include "src/utils/Dispatcher.h" // 包含 Dispatcher
#include "common/Components.h"    // 包含 Position, Size, Clickable, ButtonState, Hierarchy
#include "common/Tags.h"          // 包含 HoveredTag, ActiveTag, DisabledTag, LayoutDirtyTag
#include "src/ui/interface/Isystem.h"
#include "common/Types.h" // 包含 Vec2

namespace ui::systems
{

class InteractionSystem : public ui::interface::EnableRegister<InteractionSystem>
{
public:
    void registerHandlersImpl()
    {
        auto& dispatcher = utils::Dispatcher::getInstance();
        dispatcher.sink<ui::events::SDLEvent>().connect<&InteractionSystem::onSDLEvent>(*this);
        DetailExposed();
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

        // 获取鼠标位置（SDL API 直接获取）
        float mouseX = 0.0F, mouseY = 0.0F;
        SDL_GetMouseState(&mouseX, &mouseY);
        Vec2 mousePos(mouseX, mouseY);

        // 获取当前获焦的 SDL 窗口
        SDL_Window* focusedSDLWindow = SDL_GetMouseFocus();
        if (focusedSDLWindow == nullptr) return;

        // 查找对应的 Window 实体
        entt::entity topWindow = entt::null;
        auto view = registry.view<components::Window>();
        for (auto entity : view)
        {
            if (view.get<components::Window>(entity).windowID == SDL_GetWindowID(focusedSDLWindow))
            {
                topWindow = entity;
                break;
            }
        }

        if (topWindow == entt::null) return;

        // 获取 Z-Order 排序的可交互实体（只在当前窗口内搜索）
        auto interactables = getZOrderedInteractables(registry, topWindow);

        entt::entity hoveredEntity = entt::null;

        // 从前到后进行碰撞测试
        for (auto entity : interactables)
        {
            [[maybe_unused]] const auto& pos = registry.get<const components::Position>(entity);
            const auto& size = registry.get<const components::Size>(entity);

            Vec2 absPos = getAbsolutePosition(registry, entity);

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
     * - 识别 Quit / Window Resized，并通过回调交由上层处理
     * - 直接从 SDL 事件中追踪鼠标状态
     */
    void onSDLEvent(ui::events::SDLEvent& sdlEvent)
    {
        auto& dispatcher = ::utils::Dispatcher::getInstance();

        auto& event = sdlEvent.event;

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                    dispatcher.enqueue<ui::events::QuitRequested>();
                    break;

                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                {
                    auto& registry = ::utils::Registry::getInstance();
                    // 查找对应的 Window 实体
                    entt::entity targetWindow = entt::null;
                    auto view = registry.view<components::Window>();
                    for (auto entity : view)
                    {
                        if (view.get<components::Window>(entity).windowID == event.window.windowID)
                        {
                            targetWindow = entity;
                            break;
                        }
                    }

                    if (registry.valid(targetWindow))
                    {
                        dispatcher.enqueue<ui::events::CloseWindow>(ui::events::CloseWindow{targetWindow});
                    }
                    else
                    {
                        // 如果找不到窗口实体（可能是因为各种原因），但收到了关闭请求，保底退出
                        // 不过通常只要我们管理得当，windowID 应该能对应上
                        // 如果是最后的主窗口关闭，WidgetSystem::onCloseWindow 里面会负责发出 QuitRequested
                    }
                    break;
                }

                // 窗口暴露/需要重绘：仅触发渲染，不更新尺寸（因为 data1/data2 无效）
                case SDL_EVENT_WINDOW_EXPOSED:
                    dispatcher.trigger<ui::events::UpdateRendering>();
                    break;

                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                {
                    auto& registry = ::utils::Registry::getInstance();
                    auto view = registry.view<components::Window, components::Size>();
                    for (auto entity : view)
                    {
                        if (view.get<components::Window>(entity).windowID == event.window.windowID)
                        {
                            auto& size = view.get<components::Size>(entity);
                            size.size.x() = static_cast<float>(event.window.data1);
                            size.size.y() = static_cast<float>(event.window.data2);
                            registry.emplace_or_replace<components::LayoutDirtyTag>(entity);
                            break;
                        }
                    }
                    dispatcher.trigger<ui::events::UpdateLayout>(ui::events::UpdateLayout{});
                    dispatcher.trigger<ui::events::UpdateRendering>(ui::events::UpdateRendering{});
                    break;
                }
                case SDL_EVENT_WINDOW_MOVED:
                {
                    auto& registry = ::utils::Registry::getInstance();
                    auto view = registry.view<components::Window, components::Position>();
                    for (auto entity : view)
                    {
                        if (view.get<components::Window>(entity).windowID == event.window.windowID)
                        {
                            auto& pos = view.get<components::Position>(entity);
                            pos.value.x() = static_cast<float>(event.window.data1);
                            pos.value.y() = static_cast<float>(event.window.data2);
                            break;
                        }
                    }
                    dispatcher.trigger<ui::events::UpdateRendering>();
                    break;
                }
                case SDL_EVENT_WINDOW_RESTORED:
                case SDL_EVENT_WINDOW_MAXIMIZED:
                    dispatcher.trigger<ui::events::UpdateRendering>();
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    getInput(false, false);
                    dispatcher.trigger<ui::events::UpdateRendering>();
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        m_mouseDown = true;
                        getInput(true, false);
                    }
                    else
                    {
                        getInput(false, false);
                    }
                    break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        m_mouseDown = false;
                        getInput(false, true);
                    }
                    else
                    {
                        getInput(false, false);
                    }
                    break;
                case SDL_EVENT_MOUSE_WHEEL:
                    dispatcher.trigger<ui::events::UpdateRendering>();
                    break;
                default:
                    break;
            }
        }
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
    bool isPointInRect(const Vec2& point, const Vec2& pos, const Vec2& size) const
    {
        return point.x() >= pos.x() && point.x() < (pos.x() + size.x()) && point.y() >= pos.y() &&
               point.y() < (pos.y() + size.y());
    }

    /**
     * @brief 计算实体的绝对位置
     * @param registry 注册表
     * @param entity 当前实体
     * @return ImVec2 实体的绝对位置
     *
     */
    Vec2 getAbsolutePosition(entt::registry& registry, entt::entity entity)
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
        Vec2 pos(0.0f, 0.0f);
        for (auto it = path.rbegin(); it != path.rend(); ++it)
        {
            entt::entity e = *it;
            // 窗口本身的 Position 是屏幕坐标，在窗口内交互时应视为原点(0,0)，忽略其屏幕位置偏移
            if (registry.any_of<components::WindowTag, components::DialogTag>(e)) continue;

            const auto* posComp = registry.try_get<components::Position>(e);
            if (posComp)
            {
                pos.x() += posComp->value.x();
                pos.y() += posComp->value.y();
            }
        }
        return pos;
    }

    /**
     * @brief 获取按 Z-Order 从前到后排序的可交互实体列表
     * @param registry 注册表
     * @param topWindow 当前鼠标所在的顶层窗口（entt::null 表示不在任何窗口内）
     * @note 只返回属于 topWindow 的可交互实体
     */
    static std::vector<entt::entity> getZOrderedInteractables(entt::registry& registry, entt::entity topWindow)
    {
        std::vector<std::pair<int, entt::entity>> interactables;
        auto view = registry.view<const components::Position, const components::Size, const components::Clickable>();

        for (auto entity : view)
        {
            // 忽略禁用或不可见的实体
            if (registry.any_of<components::DisabledTag>(entity) || !registry.any_of<components::VisibleTag>(entity))
            {
                continue;
            }

            // 检查该实体是否属于 topWindow
            entt::entity rootWindow = findRootWindow(registry, entity);

            // 如果 topWindow 为 null（没有窗口），则只接受没有窗口祖先的实体
            // 如果 topWindow 不为 null，则只接受属于该窗口的实体
            if (rootWindow != topWindow) continue;

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

    /**
     * @brief 查找实体所属的根窗口/对话框
     * @return 根窗口实体，如果不在任何窗口内则返回 entt::null
     */
    static entt::entity findRootWindow(entt::registry& registry, entt::entity entity)
    {
        entt::entity current = entity;
        entt::entity rootWindow = entt::null;

        while (current != entt::null && registry.valid(current))
        {
            if (registry.any_of<components::WindowTag, components::DialogTag>(current))
            {
                rootWindow = current;
            }
            const auto* hierarchy = registry.try_get<components::Hierarchy>(current);
            current = hierarchy ? hierarchy->parent : entt::null;
        }

        return rootWindow;
    }

    /**
     * @brief 对SDL添加事件监听器的接口增加cpp模板风格封装
     * @param func {comment}
     */
    template <typename F>
    static void AddEventWatch(F func)
    {
        if constexpr (std::is_convertible_v<F, SDL_EventFilter>)
        {
            SDL_AddEventWatch(func, nullptr);
        }
        else
        {
            auto* ptr = new F(std::move(func));
            SDL_AddEventWatch(
                [](void* userdata, SDL_Event* event) -> bool
                {
                    auto& f = *static_cast<F*>(userdata);
                    if constexpr (std::is_invocable_v<F, void*, SDL_Event*>)
                    {
                        return f(userdata, event);
                    }
                    else
                    {
                        return f(event);
                    }
                },
                ptr);
        }
    }

    static void DetailExposed()
    {
        // Add event watch to handle blocking modal loops (e.g., resizing on Windows)
        SDL_AddEventWatch(
            [](void*, SDL_Event* event) -> bool
            {
                // 对于大小改变，因为是在 Windows 消息循环中阻塞发生的，需要立即更新数据模型
                if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
                {
                    auto& registry = utils::Registry::getInstance();
                    auto view = registry.view<components::Window, components::Size>();
                    for (auto entity : view)
                    {
                        if (view.get<components::Window>(entity).windowID == event->window.windowID)
                        {
                            auto& size = view.get<components::Size>(entity);
                            size.size.x() = static_cast<float>(event->window.data1);
                            size.size.y() = static_cast<float>(event->window.data2);
                            registry.emplace_or_replace<components::LayoutDirtyTag>(entity);
                            break;
                        }
                    }
                }

                // 无论是大小改变还是窗口重绘，都需要立即重新渲染以避免黑屏或卡顿
                if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED || event->type == SDL_EVENT_WINDOW_EXPOSED)
                {
                    auto& dispatcher = utils::Dispatcher::getInstance();
                    // 强制触发布局和渲染更新
                    dispatcher.trigger<ui::events::UpdateLayout>(ui::events::UpdateLayout{});
                    dispatcher.trigger<ui::events::UpdateRendering>(ui::events::UpdateRendering{});
                }
                return true;
            },
            nullptr);
    }
};

} // namespace ui::systems