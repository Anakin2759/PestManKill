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
#include "common/Events.hpp"
#include "singleton/Registry.hpp"   // 包含 Registry
#include "singleton/Dispatcher.hpp" // 包含 Dispatcher
#include "common/Components.hpp"    // 包含 Position, Size, Clickable, ButtonState, Hierarchy
#include "common/Tags.hpp"          // 包含 HoveredTag, ActiveTag, DisabledTag, LayoutDirtyTag
#include "interface/Isystem.hpp"
#include "common/Types.hpp"

namespace ui::systems
{

class InteractionSystem : public ui::interface::EnableRegister<InteractionSystem>
{
public:
    void registerHandlersImpl()
    {
        Dispatcher::Sink<ui::events::SDLEvent>().connect<&InteractionSystem::onSDLEvent>(*this);
        Dispatcher::Sink<ui::events::UpdateEvent>().connect<&InteractionSystem::onUpdate>(*this);
        DetailExposed();
    }

    void unregisterHandlersImpl() {}

private:
    /**
     * @brief 每帧更新，处理键盘长按重复输入
     */
    void onUpdate(const events::UpdateEvent&)
    {
        // 检查是否有按键长按
        if (m_heldKey != SDLK_UNKNOWN && m_keyPressTime > 0)
        {
            uint64_t currentTime = SDL_GetTicks();
            uint64_t holdDuration = currentTime - m_keyPressTime;

            // 超过延迟时间，开始重复输入
            if (holdDuration >= KEY_REPEAT_DELAY)
            {
                uint64_t timeSinceLastRepeat = currentTime - m_lastRepeatTime;

                // 达到重复间隔，执行重复输入
                if (timeSinceLastRepeat >= KEY_REPEAT_INTERVAL)
                {
                    handleKeyDown(m_heldKey);
                    m_lastRepeatTime = currentTime;
                    Dispatcher::Trigger<ui::events::UpdateRendering>();
                }
            }
        }
    }

    /**
     * @brief 处理每一帧输入事件和交互状态更新
     * @param mousePressed 本帧是否有鼠标按下事件（来自 SDL）
     * @param mouseReleased 本帧是否有鼠标释放事件（来自 SDL）
     */
    void getInput(bool mousePressed, bool mouseReleased) noexcept
    {
        // 获取鼠标位置（SDL API 直接获取）
        float mouseX = 0.0F, mouseY = 0.0F;
        SDL_GetMouseState(&mouseX, &mouseY);
        Vec2 mousePos(mouseX, mouseY);

        // 获取当前获焦的 SDL 窗口
        SDL_Window* focusedSDLWindow = SDL_GetMouseFocus();
        if (focusedSDLWindow == nullptr) return;

        // 查找对应的 Window 实体
        entt::entity topWindow = entt::null;
        auto view = Registry::View<components::Window>();
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
        auto interactables = getZOrderedInteractables(topWindow);

        entt::entity hoveredEntity = entt::null;

        // 从前到后进行碰撞测试
        for (auto entity : interactables)
        {
            [[maybe_unused]] const auto& pos = Registry::Get<components::Position>(entity);
            const auto& size = Registry::Get<components::Size>(entity);

            Vec2 absPos = getAbsolutePosition(entity);

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
            if (m_hoveredEntity != entt::null && Registry::Valid(m_hoveredEntity))
            {
                Registry::Remove<components::HoveredTag>(m_hoveredEntity);
                Dispatcher::Enqueue<events::UnhoverEvent>(events::UnhoverEvent{m_hoveredEntity});
            }

            // 触发 HoverEvent（如果现在有新的悬浮实体）
            if (hoveredEntity != entt::null)
            {
                Registry::EmplaceOrReplace<components::HoveredTag>(hoveredEntity);
                Dispatcher::Enqueue<events::HoverEvent>(events::HoverEvent{hoveredEntity});
            }

            m_hoveredEntity = hoveredEntity;
        }

        // 处理鼠标按下（使用 SDL 事件驱动的状态）
        if (mousePressed && hoveredEntity != entt::null)
        {
            if (m_activeEntity == entt::null)
            {
                m_activeEntity = hoveredEntity;
                Registry::EmplaceOrReplace<components::ActiveTag>(m_activeEntity);

                // 触发按下事件
                if (Registry::AnyOf<components::Pressable>(m_activeEntity))
                {
                    Dispatcher::Enqueue<events::MousePressEvent>(events::MousePressEvent{m_activeEntity});
                }
            }
        }

        // 处理鼠标释放（使用 SDL 事件驱动的状态）
        if (mouseReleased && m_activeEntity != entt::null)
        {
            // 触发松开事件（在移除 ActiveTag 之前）
            if (Registry::AnyOf<components::Pressable>(m_activeEntity))
            {
                Dispatcher::Enqueue<events::MouseReleaseEvent>(events::MouseReleaseEvent{m_activeEntity});
            }

            Registry::Remove<components::ActiveTag>(m_activeEntity);

            // 如果释放时仍然在同一实体上，触发点击事件
            if (m_activeEntity == hoveredEntity)
            {
                if (Registry::TryGet<components::Clickable>(m_activeEntity) != nullptr)
                {
                    Dispatcher::Enqueue<events::ClickEvent>(events::ClickEvent{m_activeEntity});
                }
                // 处理焦点切换
                handleFocusChange(m_activeEntity, focusedSDLWindow);
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
        SDL_Event event{};

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                    Dispatcher::Enqueue<ui::events::QuitRequested>();
                    break;

                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                {
                    // 查找对应的 Window 实体
                    entt::entity targetWindow = entt::null;
                    auto view = Registry::View<components::Window>();
                    for (auto entity : view)
                    {
                        if (view.get<components::Window>(entity).windowID == event.window.windowID)
                        {
                            targetWindow = entity;
                            break;
                        }
                    }

                    if (Registry::Valid(targetWindow))
                    {
                        Dispatcher::Enqueue<ui::events::CloseWindow>(ui::events::CloseWindow{targetWindow});
                    }
                    break;
                }

                case SDL_EVENT_WINDOW_EXPOSED:
                    Dispatcher::Trigger<ui::events::UpdateRendering>();
                    break;

                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                {
                    Dispatcher::Trigger<ui::events::WindowPixelSizeChanged>(ui::events::WindowPixelSizeChanged{
                        event.window.windowID, event.window.data1, event.window.data2});
                    Dispatcher::Trigger<ui::events::UpdateLayout>(ui::events::UpdateLayout{});
                    Dispatcher::Trigger<ui::events::UpdateRendering>(ui::events::UpdateRendering{});
                    break;
                }
                case SDL_EVENT_WINDOW_MOVED:
                {
                    Dispatcher::Trigger<ui::events::WindowMoved>(
                        ui::events::WindowMoved{event.window.windowID, event.window.data1, event.window.data2});
                    Dispatcher::Trigger<ui::events::UpdateRendering>();
                    break;
                }
                case SDL_EVENT_WINDOW_RESTORED:
                case SDL_EVENT_WINDOW_MAXIMIZED:
                    Dispatcher::Trigger<ui::events::UpdateRendering>();
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                {
                    // 处理滚动条拖拽
                    if (m_isDraggingScrollbar && m_scrollingEntity != entt::null)
                    {
                        handleScrollbarDrag(event.motion.y);
                        Dispatcher::Trigger<ui::events::UpdateRendering>();
                    }
                    else
                    {
                        getInput(false, false);
                        Dispatcher::Trigger<ui::events::UpdateRendering>();
                    }
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        m_mouseDown = true;
                        // 优先检查是否点击了滚动条
                        if (checkScrollbarHit(event.button.x, event.button.y))
                        {
                            // 滚动条捕获输入，不传播给其他实体
                            Dispatcher::Trigger<ui::events::UpdateRendering>();
                        }
                        else
                        {
                            getInput(true, false);
                        }
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
                        if (m_isDraggingScrollbar)
                        {
                            m_isDraggingScrollbar = false;
                            m_scrollingEntity = entt::null;
                            Dispatcher::Trigger<ui::events::UpdateRendering>();
                        }
                        else
                        {
                            getInput(false, true);
                        }
                    }
                    else
                    {
                        getInput(false, false);
                    }
                    break;
                case SDL_EVENT_TEXT_INPUT:
                    handleTextInput(event.text.text);
                    Dispatcher::Trigger<ui::events::UpdateRendering>();
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (!event.key.repeat) // 只处理真实按下，忽略系统重复
                    {
                        m_heldKey = event.key.key;
                        m_keyPressTime = SDL_GetTicks();
                        m_lastRepeatTime = m_keyPressTime;
                        handleKeyDown(event.key.key);
                    }
                    Dispatcher::Trigger<ui::events::UpdateRendering>();
                    break;
                case SDL_EVENT_KEY_UP:
                    // 重置长按状态
                    if (event.key.key == m_heldKey)
                    {
                        m_heldKey = SDLK_UNKNOWN;
                        m_keyPressTime = 0;
                        m_lastRepeatTime = 0;
                    }
                    break;
                case SDL_EVENT_MOUSE_WHEEL:
                    handleWheelEvent(event.wheel);
                    Dispatcher::Trigger<ui::events::UpdateRendering>();
                    break;
                default:
                    break;
            }
        }
    }

    // =======================================================
    // Scroll Interaction Helpers
    // =======================================================
    /**
     * @brief 处理鼠标滚轮事件
     * @param event SDL_MouseWheelEvent 引用
     */
    void handleWheelEvent(const SDL_MouseWheelEvent& event)
    {
        entt::entity target = entt::null;

        // 1. 尝试从当前悬停实体向上查找 ScrollArea
        entt::entity current = m_hoveredEntity;
        while (current != entt::null && Registry::Valid(current))
        {
            if (Registry::AnyOf<components::ScrollArea>(current))
            {
                target = current;
                break;
            }
            if (const auto* hierarchy = Registry::TryGet<components::Hierarchy>(current))
            {
                current = hierarchy->parent;
            }
            else
            {
                current = entt::null;
            }
        }

        // 2. 如果没找到，进行空间搜索（鼠标所在位置的 ScrollArea）
        if (target == entt::null)
        {
            float mouseX = 0.0f;
            float mouseY = 0.0f;
            SDL_GetMouseState(&mouseX, &mouseY);
            Vec2 mousePos(mouseX, mouseY);

            auto view = Registry::
                View<components::ScrollArea, components::Size, components::VisibleTag, components::Position>();
            for (auto entity : view)
            {
                const auto& size = view.get<components::Size>(entity);
                Vec2 absPos = getAbsolutePosition(entity);
                if (isPointInRect(mousePos, absPos, size.size))
                {
                    target = entity;
                    break; // 找到即止
                }
            }
        }

        if (target != entt::null)
        {
            auto& scroll = Registry::Get<components::ScrollArea>(target);
            const auto& size = Registry::Get<components::Size>(target);

            // 滚动方向：wheel.y > 0 表示向上滚，内容应向下移（offset 减小）
            float step = 30.0f;
            float delta = -event.y * step;

            scroll.scrollOffset.y() += delta;

            // 限制范围
            float maxScroll = std::max(0.0f, scroll.contentSize.y() - size.size.y());
            scroll.scrollOffset.y() = std::clamp(scroll.scrollOffset.y(), 0.0f, maxScroll);
        }
    }

    bool checkScrollbarHit(float mouseX, float mouseY)
    {
        // 查找所有可见的 ScrollArea，检查点击位置是否在它们的滚动条区域
        auto view =
            Registry::View<components::ScrollArea, components::Size, components::VisibleTag, components::Position>();

        for (auto entity : view)
        {
            if (isPointInScrollbar(entity, Vec2(mouseX, mouseY)))
            {
                m_scrollingEntity = entity;
                m_isDraggingScrollbar = true;
                const auto& scroll = Registry::Get<components::ScrollArea>(entity);
                m_dragStartScrollY = scroll.scrollOffset.y();
                m_dragStartY = mouseY;
                return true;
            }
        }
        return false;
    }
    /**
     * @brief 处理滚动条区域的点碰撞测试
     * @param entity 实体
     * @param mousePos 鼠标绝对位置
     * @return true 命中滚动条区域
     * @return false 未命中滚动条区域
     */
    bool isPointInScrollbar(entt::entity entity, const Vec2& mousePos)
    {
        const auto& scrollArea = Registry::Get<components::ScrollArea>(entity);
        const auto& size = Registry::Get<components::Size>(entity);
        Vec2 pos = getAbsolutePosition(entity);

        bool hasVerticalScroll =
            (scrollArea.scroll == policies::Scroll::Vertical || scrollArea.scroll == policies::Scroll::Both);
        bool showScrollbars = (scrollArea.showScrollbars != policies::ScrollBarVisibility::AlwaysOff);

        // 仅处理垂直滚动条
        if (hasVerticalScroll && showScrollbars && scrollArea.contentSize.y() > size.size.y())
        {
            // 滚动条区域判定：右侧 12px 宽的区域
            float barWidth = 12.0f;
            float trackX = pos.x() + size.size.x() - barWidth;
            float trackY = pos.y();
            float trackH = size.size.y();

            return (mousePos.x() >= trackX && mousePos.x() <= trackX + barWidth && mousePos.y() >= trackY &&
                    mousePos.y() <= trackY + trackH);
        }
        return false;
    }
    /**
     * @brief 处理滚动条拖拽
     * @param mouseY 当前鼠标Y位置
     */
    void handleScrollbarDrag(float mouseY)
    {
        if (m_scrollingEntity == entt::null) return;

        if (!Registry::Valid(m_scrollingEntity)) return;

        auto& scroll = Registry::Get<components::ScrollArea>(m_scrollingEntity);
        const auto& size = Registry::Get<components::Size>(m_scrollingEntity);

        float deltaY = mouseY - m_dragStartY;

        float trackSize = size.size.y();
        float visibleRatio = std::min(1.0f, size.size.y() / scroll.contentSize.y());
        float thumbSize = std::max(20.0f, trackSize * visibleRatio);
        float trackScrollableArea = trackSize - thumbSize;
        float maxScroll = scroll.contentSize.y() - size.size.y();

        if (trackScrollableArea > 1.0f && maxScroll > 0.0f)
        {
            float scrollDelta = deltaY * (maxScroll / trackScrollableArea);
            float newScroll = m_dragStartScrollY + scrollDelta;
            scroll.scrollOffset.y() = std::clamp(newScroll, 0.0f, maxScroll);
        }
    }

    entt::entity m_activeEntity = entt::null;  // 当前处于 Active (鼠标按下) 状态的实体
    entt::entity m_hoveredEntity = entt::null; // 当前悬浮的实体
    bool m_mouseDown = false;                  // 鼠标按下状态（基于 SDL 事件追踪）

    // 滚动条交互状态
    entt::entity m_scrollingEntity = entt::null; // 当前正在拖拽滚动条的实体
    bool m_isDraggingScrollbar = false;          // 是否正在拖拽滚动条
    float m_dragStartY = 0.0f;                   // 拖拽开始时的鼠标Y位置
    float m_dragStartScrollY = 0.0f;             // 拖拽开始时的滚动偏移

    // 键盘长按状态
    SDL_Keycode m_heldKey = SDLK_UNKNOWN;               // 当前按下的按键
    uint64_t m_keyPressTime = 0;                        // 按键按下时间（毫秒）
    uint64_t m_lastRepeatTime = 0;                      // 上次重复输入时间
    static constexpr uint64_t KEY_REPEAT_DELAY = 500;   // 长按触发延迟（毫秒）
    static constexpr uint64_t KEY_REPEAT_INTERVAL = 50; // 重复输入间隔（毫秒）

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
     * @param entity 当前实体
     * @return ImVec2 实体的绝对位置
     *
     */
    Vec2 getAbsolutePosition(entt::entity entity)
    {
        // 构建从当前实体到根的路径
        std::vector<entt::entity> path;
        entt::entity current = entity;
        while (current != entt::null && Registry::Valid(current))
        {
            path.push_back(current);
            const auto* hierarchy = Registry::TryGet<components::Hierarchy>(current);
            current = hierarchy ? hierarchy->parent : entt::null;
        }

        // 从根到当前实体正向遍历，模拟 RenderSystem 的递归逻辑
        Vec2 pos(0.0f, 0.0f);
        for (auto it = path.rbegin(); it != path.rend(); ++it)
        {
            entt::entity e = *it;
            // 窗口本身的 Position 是屏幕坐标，在窗口内交互时应视为原点(0,0)，忽略其屏幕位置偏移
            if (Registry::AnyOf<components::WindowTag, components::DialogTag>(e)) continue;

            const auto* posComp = Registry::TryGet<components::Position>(e);
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
     * @param topWindow 当前鼠标所在的顶层窗口（entt::null 表示不在任何窗口内）
     * @note 只返回属于 topWindow 的可交互实体
     */
    static std::vector<entt::entity> getZOrderedInteractables(entt::entity topWindow)
    {
        std::vector<std::pair<int, entt::entity>> interactables;
        // 修改：遍历所有具有 Position 和 Size 的实体，单独检查交互组件
        auto view = Registry::View<components::Position, components::Size>();

        for (auto entity : view)
        {
            // 筛选条件：必须是 Clickable 或可编辑的 TextEdit (输入框)
            bool isInteractive = Registry::AnyOf<components::Clickable>(entity);
            if (!isInteractive && Registry::AnyOf<components::TextEditTag>(entity))
            {
                const auto* edit = Registry::TryGet<components::TextEdit>(entity);
                if (edit != nullptr && !policies::HasFlag(edit->inputMode, policies::TextFlag::ReadOnly))
                {
                    isInteractive = true;
                }
            }
            if (!isInteractive) continue;

            // 忽略禁用或不可见的实体
            if (Registry::AnyOf<components::DisabledTag>(entity) || !Registry::AnyOf<components::VisibleTag>(entity))
            {
                continue;
            }

            // 检查该实体是否属于 topWindow
            entt::entity rootWindow = findRootWindow(entity);

            // 如果 topWindow 为 null（没有窗口），则只接受没有窗口祖先的实体
            // 如果 topWindow 不为 null，则只接受属于该窗口的实体
            if (rootWindow != topWindow) continue;

            // 简单深度排序：层级越深（子元素），Z-Order 越高
            int depth = 0;
            entt::entity current = entity;
            while (current != entt::null)
            {
                const auto* hierarchy = Registry::TryGet<components::Hierarchy>(current);
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
    static entt::entity findRootWindow(entt::entity entity)
    {
        entt::entity current = entity;
        entt::entity rootWindow = entt::null;

        while (current != entt::null && Registry::Valid(current))
        {
            if (Registry::AnyOf<components::WindowTag, components::DialogTag>(current))
            {
                rootWindow = current;
            }
            const auto* hierarchy = Registry::TryGet<components::Hierarchy>(current);
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
                // 对于大小改变，因为是在 Windows 消息循环中阻塞发生的，需要立即触发事件
                if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
                {
                    Dispatcher::Trigger<ui::events::WindowPixelSizeChanged>(ui::events::WindowPixelSizeChanged{
                        event->window.windowID, event->window.data1, event->window.data2});
                }
                else if (event->type == SDL_EVENT_WINDOW_MOVED)
                {
                    Dispatcher::Trigger<ui::events::WindowMoved>(
                        ui::events::WindowMoved{event->window.windowID, event->window.data1, event->window.data2});
                }

                // 无论是大小改变还是窗口重绘，都需要立即重新渲染以避免黑屏或卡顿
                if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED || event->type == SDL_EVENT_WINDOW_EXPOSED)
                {
                    // 强制触发布局和渲染更新
                    Dispatcher::Trigger<ui::events::UpdateLayout>(ui::events::UpdateLayout{});
                    Dispatcher::Trigger<ui::events::UpdateRendering>(ui::events::UpdateRendering{});
                }
                return true;
            },
            nullptr);
    }

    void handleFocusChange(entt::entity clickedEntity, SDL_Window* sdlWindow)
    {
        // 1. 如果点击的是 InputText (具有 TextEditTag)
        if (Registry::AnyOf<components::TextEditTag>(clickedEntity))
        {
            const auto* edit = Registry::TryGet<components::TextEdit>(clickedEntity);
            if (edit != nullptr && policies::HasFlag(edit->inputMode, policies::TextFlag::ReadOnly))
            {
                clearFocus();
                if (sdlWindow) SDL_StopTextInput(sdlWindow);
                return;
            }
            // 如果尚未获焦，则设为焦点
            if (!Registry::AnyOf<components::FocusedTag>(clickedEntity))
            {
                // 清除其他实体的焦点
                clearFocus();

                // 设置新焦点
                Registry::EmplaceOrReplace<components::FocusedTag>(clickedEntity);

                // 开启文本输入模式
                if (sdlWindow)
                {
                    SDL_StartTextInput(sdlWindow);

                    // 设置输入法位置
                    Vec2 absPos = getAbsolutePosition(clickedEntity);
                    const auto& size = Registry::Get<components::Size>(clickedEntity);

                    SDL_Rect rect;
                    rect.x = static_cast<int>(absPos.x());
                    rect.y = static_cast<int>(absPos.y());
                    rect.w = static_cast<int>(size.size.x());
                    rect.h = static_cast<int>(size.size.y());

                    SDL_SetTextInputArea(sdlWindow, &rect, 0);
                }
            }
        }
        else
        {
            // 2. 如果点击的是其他可交互实体(但不是输入框)
            // 清除焦点并停止输入
            clearFocus();
            if (sdlWindow) SDL_StopTextInput(sdlWindow);
        }
    }

    void clearFocus()
    {
        auto view = Registry::View<components::FocusedTag>();
        for (auto entity : view)
        {
            Registry::Remove<components::FocusedTag>(entity);
        }
    }

    void handleTextInput(const char* text)
    {
        auto view = Registry::View<components::FocusedTag, components::TextEdit, components::Text>();
        for (auto entity : view)
        {
            if (!Registry::AnyOf<components::TextEditTag>(entity)) continue;

            auto& edit = view.get<components::TextEdit>(entity);
            if (policies::HasFlag(edit.inputMode, policies::TextFlag::ReadOnly)) continue;

            std::string input = text != nullptr ? std::string(text) : std::string();
            if (input.empty()) continue;

            // 单行输入框过滤换行
            // 使用底层类型转换进行位运算检查
            const auto modeVal = static_cast<uint8_t>(edit.inputMode);
            const auto multiFlag = static_cast<uint8_t>(policies::TextFlag::Multiline);

            if ((modeVal & multiFlag) == 0)
            {
                input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());
                input.erase(std::remove(input.begin(), input.end(), '\r'), input.end());
                if (input.empty()) continue;
            }

            // 限制最大长度（按字节计）
            if (edit.buffer.size() + input.size() > edit.maxLength)
            {
                const size_t remain = edit.maxLength > edit.buffer.size() ? edit.maxLength - edit.buffer.size() : 0;
                if (remain == 0) continue;
                input.resize(remain);
            }

            edit.buffer += input;

            // 同步渲染文本
            auto& textComp = view.get<components::Text>(entity);
            textComp.content = edit.buffer;

            // 标记为 Dirty
            Registry::EmplaceOrReplace<ui::components::LayoutDirtyTag>(entity);
        }
    }

    void handleKeyDown(SDL_Keycode key)
    {
        auto view = Registry::View<components::FocusedTag, components::TextEdit, components::Text>();
        for (auto entity : view)
        {
            if (!Registry::AnyOf<components::TextEditTag>(entity)) continue;

            auto& edit = view.get<components::TextEdit>(entity);
            if (policies::HasFlag(edit.inputMode, policies::TextFlag::ReadOnly)) continue;

            auto& textComp = view.get<components::Text>(entity);

            if (key == SDLK_BACKSPACE && !edit.buffer.empty())
            {
                // Unicode感知回格处理
                while (!edit.buffer.empty())
                {
                    char c = edit.buffer.back();
                    edit.buffer.pop_back();
                    if ((c & 0xC0) != 0x80) break;
                }
                textComp.content = edit.buffer;
                Registry::EmplaceOrReplace<ui::components::LayoutDirtyTag>(entity);
            }
            else if (key == SDLK_DELETE)
            {
                // 删除键 - 删除光标后的字符（当前实现无光标位置，忽略）
            }
            else if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
            {
                const auto modeVal = static_cast<uint8_t>(edit.inputMode);
                const auto multiFlag = static_cast<uint8_t>(policies::TextFlag::Multiline);

                if ((modeVal & multiFlag) != 0)
                {
                    if (edit.buffer.size() + 1 <= edit.maxLength)
                    {
                        edit.buffer.push_back('\n');
                        textComp.content = edit.buffer;
                        Registry::EmplaceOrReplace<ui::components::LayoutDirtyTag>(entity);
                    }
                }
                else
                {
                    // 单行输入可在这里触发提交事件（当前无事件）
                }
            }
            // 注意：普通字符输入由 SDL_EVENT_TEXT_INPUT 处理，不在这里处理
        }
    }
};

} // namespace ui::systems