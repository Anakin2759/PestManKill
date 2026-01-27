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
    同步窗口属性到ECS组件（如Resizable、Frameless等）。
    处理事件：点击/焦点切换/关闭/销毁等。

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
#include <cmath>
#include <cfloat>
#include <string>
#include "../singleton/Registry.hpp"   // 包含 Registry
#include "../singleton/Dispatcher.hpp" // 包含 Dispatcher
#include "../common/Components.hpp"    // 包含 Size
#include "../common/Tags.hpp"          // 包含 LayoutDirtyTag
#include "../common/Policies.hpp"      // 包含 WindowFlag, HasFlag
#include "../interface/Isystem.hpp"
#include "../common/Events.hpp"
#include <SDL3/SDL.h>
namespace ui::systems
{

class WidgetSystem : public ui::interface::EnableRegister<WidgetSystem>
{
public:
    void registerHandlersImpl()
    {
        Dispatcher::Sink<events::CloseWindow>().connect<&WidgetSystem::onCloseWindow>(*this);
        Dispatcher::Sink<events::WindowPixelSizeChanged>().connect<&WidgetSystem::onWindowPixelSizeChanged>(*this);
        Dispatcher::Sink<events::WindowMoved>().connect<&WidgetSystem::onWindowMoved>(*this);
    }

    void unregisterHandlersImpl()
    {
        auto& dispatcher = Dispatcher::getInstance();
        Dispatcher::Sink<events::CloseWindow>().disconnect<&WidgetSystem::onCloseWindow>(*this);
        Dispatcher::Sink<events::WindowPixelSizeChanged>().disconnect<&WidgetSystem::onWindowPixelSizeChanged>(*this);
        Dispatcher::Sink<events::WindowMoved>().disconnect<&WidgetSystem::onWindowMoved>(*this);
    }

    void onCloseWindow(const events::CloseWindow& event)
    {
        // 调用递归销毁
        if (Registry::Valid(event.entity))
        {
            destroyWidget(event.entity);
        }

        if (Registry::View<components::Window>().empty())
        {
            // 没有窗口实体了，发出退出请求
            Dispatcher::Trigger<events::QuitRequested>();
        }
    }

    /**
     * @brief 处理窗口尺寸变化事件
     */
    void onWindowPixelSizeChanged(const events::WindowPixelSizeChanged& event)
    {
        auto& registry = Registry::getInstance();
        auto view = Registry::View<components::Window, components::Size>();

        for (auto entity : view)
        {
            if (view.get<components::Window>(entity).windowID == event.windowID)
            {
                auto& size = view.get<components::Size>(entity);
                size.size.x() = static_cast<float>(event.width);
                size.size.y() = static_cast<float>(event.height);
                Registry::EmplaceOrReplace<components::LayoutDirtyTag>(entity);
                break;
            }
        }
    }

    /**
     * @brief 处理窗口位置变化事件
     */
    void onWindowMoved(const events::WindowMoved& event)
    {
        auto& registry = Registry::getInstance();
        auto view = Registry::View<components::Window, components::Position>();

        for (auto entity : view)
        {
            if (view.get<components::Window>(entity).windowID == event.windowID)
            {
                auto& pos = view.get<components::Position>(entity);
                pos.value.x() = static_cast<float>(event.x);
                pos.value.y() = static_cast<float>(event.y);
                break;
            }
        }
    }

    static void onRemoveWidget(entt::entity entity) { destroyWidget(entity); }

    /**
     * @brief 手动标记实体及其父链需要重新布局（用于直接修改组件后）
     *
     * @example
     * auto& size = Registry::Get<Size>(entity);
     * size.size.x() = 100;
     * WidgetSystem::InvalidateLayout(registry, entity); // 手动标记脏
     */
    static void invalidateLayout(entt::entity entity)
    {
        entt::entity current = entity;
        while (current != entt::null && Registry::Valid(current))
        {
            Registry::EmplaceOrReplace<components::LayoutDirtyTag>(current);
            const auto* hierarchy = Registry::TryGet<components::Hierarchy>(current);
            current = hierarchy != nullptr ? hierarchy->parent : entt::null;
        }
    }

    static void invalidateRender(entt::entity entity)
    {
        // 目前仅标记自身需要重新渲染
        if (Registry::Valid(entity))
        {
            Registry::EmplaceOrReplace<components::RenderDirtyTag>(entity);
        }
    }

    /**
     * @brief 按照父子层级销毁控件实体及其子实体，并清理SDL资源
     */
    static void destroyWidget(entt::entity entity)
    {
        auto& registry = Registry::getInstance();
        if (!Registry::Valid(entity)) return;

        // 1. 递归销毁子节点
        if (auto* hierarchy = Registry::TryGet<components::Hierarchy>(entity))
        {
            // 复制一份子节点列表，防止在销毁过程中因引用失效导致崩溃
            auto children = hierarchy->children;
            for (auto child : children)
            {
                destroyWidget(child);
            }
        }

        // 2. 如果是窗口，查找并销毁关联的 SDL_Window 资源
        if (auto* windowComp = Registry::TryGet<components::Window>(entity))
        {
            // 通过 WindowID 找回 SDL_Window 指针
            if (SDL_Window* sdlWindow = SDL_GetWindowFromID(windowComp->windowID))
            {
                // 通知 RenderSystem 解绑上下文（尽管 RenderSystem 可能目前未执行动作，保留逻辑完整性）
                Dispatcher::Trigger<events::WindowGraphicsContextUnsetEvent>(
                    events::WindowGraphicsContextUnsetEvent{entity});

                SDL_DestroyWindow(sdlWindow);
            }
        }

        // 3. 最后销毁实体本身
        Registry::Destroy(entity);
    }

    static void setSize() {}

    /**
     * @brief 根据实体组件同步调整 SDL 窗口属性
     *
     * 支持同步的属性：
     * - 窗口标题 (Title 组件或 Window::title)
     * - 窗口位置 (Position 组件，支持自动居中)
     * - 窗口大小 (Size 组件)
     * - 窗口大小约束 (Window::minSize/maxSize)
     * - 窗口可调整大小 (WindowFlag::NoResize)
     * - 窗口透明度 (Alpha 组件)
     * - 窗口可见性 (VisibleTag)
     * - 模态属性 (WindowFlag::Modal) - 用于 Dialog
     */
    static void syncSDLWindowProperties(entt::entity entity, components::Window& windowComp, SDL_Window* sdlWindow)
    {
        if (sdlWindow == nullptr) return;

        // 1. 同步窗口标题
        syncWindowTitle(entity, windowComp, sdlWindow);

        // 2. 同步窗口位置
        syncWindowPosition(entity, sdlWindow);

        // 3. 同步窗口大小约束
        syncWindowSizeConstraints(windowComp, sdlWindow);

        // 4. 同步窗口可调整大小属性
        syncWindowResizable(windowComp, sdlWindow);

        // 4.1 同步窗口无边框属性
        syncWindowFrameless(windowComp, sdlWindow);

        // 5. 同步窗口透明度
        syncWindowOpacity(entity, sdlWindow);

        // 6. 同步窗口可见性
        syncWindowVisibility(entity, sdlWindow);

        // 7. 同步模态属性 (Dialog)
        syncWindowModal(entity, windowComp, sdlWindow);
    }

    /**
     * @brief 同步窗口大小（保留但不再自动调用，供外部需要时使用）
     */
    static void syncWindowSize(entt::entity entity, SDL_Window* sdlWindow)
    {
        const auto* sizeComp = Registry::TryGet<components::Size>(entity);
        if (sizeComp == nullptr) return;

        // 只在非自动大小模式下同步
        if (policies::HasFlag(sizeComp->sizePolicy, policies::Size::HAuto) ||
            policies::HasFlag(sizeComp->sizePolicy, policies::Size::VAuto))
            return;

        int currentW = 0;
        int currentH = 0;
        SDL_GetWindowSize(sdlWindow, &currentW, &currentH);

        int targetW = static_cast<int>(sizeComp->size.x());
        int targetH = static_cast<int>(sizeComp->size.y());

        // 只在大小不同时才设置，避免不必要的窗口操作
        if (currentW != targetW || currentH != targetH)
        {
            SDL_SetWindowSize(sdlWindow, targetW, targetH);
        }
    }

    /**
     * @brief 同步窗口位置
     *
     * 逻辑：
     * - 首次渲染时，从 SDL 窗口读取实际位置并更新 Position 组件
     * - 之后只在 Position 组件被代码修改时才更新窗口位置
     */
    static void syncWindowPosition(entt::entity entity, SDL_Window* sdlWindow)
    {
        auto* posComp = Registry::TryGet<components::Position>(entity);
        if (posComp == nullptr) return;

        // 获取当前窗口实际位置
        int currentX = 0;
        int currentY = 0;
        SDL_GetWindowPosition(sdlWindow, &currentX, &currentY);

        // 如果 Position 组件是默认值 (0, 0)，说明是首次渲染，从窗口同步位置
        constexpr float EPSILON = 0.01F;
        if (std::abs(posComp->value.x()) < EPSILON && std::abs(posComp->value.y()) < EPSILON)
        {
            // 更新 Position 组件为窗口实际位置
            posComp->value = Eigen::Vector2f{static_cast<float>(currentX), static_cast<float>(currentY)};
            return;
        }

        // 后续帧：只在 Position 组件与实际窗口位置不同时才设置
        int targetX = static_cast<int>(posComp->value.x());
        int targetY = static_cast<int>(posComp->value.y());

        // 只在位置明显不同时才设置（避免浮点误差导致的抖动）
        if (std::abs(currentX - targetX) > 1 || std::abs(currentY - targetY) > 1)
        {
            SDL_SetWindowPosition(sdlWindow, targetX, targetY);
        }
    }

    /**
     * @brief 同步窗口标题
     */
    static void syncWindowTitle(entt::entity entity, const components::Window& windowComp, SDL_Window* sdlWindow)
    {
        std::string newTitle;

        // 优先使用 Title 组件
        const auto* titleComp = Registry::TryGet<components::Title>(entity);
        if (titleComp != nullptr && !titleComp->text.empty())
        {
            newTitle = titleComp->text;
        }
        else if (!windowComp.title.empty())
        {
            newTitle = windowComp.title;
        }

        if (!newTitle.empty())
        {
            const char* currentTitle = SDL_GetWindowTitle(sdlWindow);
            if (currentTitle == nullptr || newTitle != currentTitle)
            {
                SDL_SetWindowTitle(sdlWindow, newTitle.c_str());
            }
        }
    }

    /**
     * @brief 同步窗口大小约束
     */
    static void syncWindowSizeConstraints(const components::Window& windowComp, SDL_Window* sdlWindow)
    {
        int currentMinW = 0;
        int currentMinH = 0;
        int currentMaxW = 0;
        int currentMaxH = 0;
        SDL_GetWindowMinimumSize(sdlWindow, &currentMinW, &currentMinH);
        SDL_GetWindowMaximumSize(sdlWindow, &currentMaxW, &currentMaxH);

        int newMinW = static_cast<int>(windowComp.minSize.x());
        int newMinH = static_cast<int>(windowComp.minSize.y());
        int newMaxW = (windowComp.maxSize.x() < FLT_MAX) ? static_cast<int>(windowComp.maxSize.x()) : 0;
        int newMaxH = (windowComp.maxSize.y() < FLT_MAX) ? static_cast<int>(windowComp.maxSize.y()) : 0;

        if (newMinW != currentMinW || newMinH != currentMinH)
        {
            SDL_SetWindowMinimumSize(sdlWindow, newMinW, newMinH);
        }

        if (newMaxW != currentMaxW || newMaxH != currentMaxH)
        {
            SDL_SetWindowMaximumSize(sdlWindow, newMaxW, newMaxH);
        }
    }

    /**
     * @brief 同步窗口无边框属性 (NoTitleBar)
     */
    static void syncWindowFrameless(const components::Window& windowComp, SDL_Window* sdlWindow)
    {
        SDL_WindowFlags flags = SDL_GetWindowFlags(sdlWindow);
        bool currentlyBordered = (flags & SDL_WINDOW_BORDERLESS) == 0;
        bool shouldBeBordered = !policies::HasFlag(windowComp.flags, policies::WindowFlag::NoTitleBar);

        if (currentlyBordered != shouldBeBordered)
        {
            SDL_SetWindowBordered(sdlWindow, shouldBeBordered);
        }
    }

    /**
     * @brief 同步窗口可调整大小属性
     */
    static void syncWindowResizable(const components::Window& windowComp, SDL_Window* sdlWindow)
    {
        SDL_WindowFlags flags = SDL_GetWindowFlags(sdlWindow);
        bool currentlyResizable = (flags & SDL_WINDOW_RESIZABLE) != 0;
        bool shouldBeResizable = !policies::HasFlag(windowComp.flags, policies::WindowFlag::NoResize);

        if (currentlyResizable != shouldBeResizable)
        {
            SDL_SetWindowResizable(sdlWindow, shouldBeResizable);
        }
    }

    /**
     * @brief 同步窗口透明度
     */
    static void syncWindowOpacity(entt::entity entity, SDL_Window* sdlWindow)
    {
        const auto* alphaComp = Registry::TryGet<components::Alpha>(entity);
        if (alphaComp != nullptr)
        {
            float currentOpacity = SDL_GetWindowOpacity(sdlWindow);
            // 只在透明度差异超过阈值时更新，避免频繁调用
            constexpr float OPACITY_THRESHOLD = 0.01F;
            if (std::abs(currentOpacity - alphaComp->value) > OPACITY_THRESHOLD)
            {
                SDL_SetWindowOpacity(sdlWindow, alphaComp->value);
            }
        }
    }

    /**
     * @brief 同步窗口可见性
     */
    static void syncWindowVisibility(entt::entity entity, SDL_Window* sdlWindow)
    {
        bool shouldBeVisible = Registry::AnyOf<components::VisibleTag>(entity);
        SDL_WindowFlags flags = SDL_GetWindowFlags(sdlWindow);
        bool currentlyVisible = (flags & SDL_WINDOW_HIDDEN) == 0;

        if (shouldBeVisible && !currentlyVisible)
        {
            SDL_ShowWindow(sdlWindow);
        }
        else if (!shouldBeVisible && currentlyVisible)
        {
            SDL_HideWindow(sdlWindow);
        }
    }

    /**
     * @brief 同步模态属性 (用于 Dialog)
     */
    static void syncWindowModal(entt::entity entity, const components::Window& windowComp, SDL_Window* sdlWindow)
    {
        // 检查是否是 Dialog 实体
        bool isDialog = Registry::AnyOf<components::DialogTag>(entity);
        if (!isDialog) return;

        SDL_WindowFlags flags = SDL_GetWindowFlags(sdlWindow);
        bool currentlyModal = (flags & SDL_WINDOW_MODAL) != 0;

        bool isModal = policies::HasFlag(windowComp.flags, policies::WindowFlag::Modal);

        if (isModal && !currentlyModal)
        {
            // 设置为模态窗口
            SDL_SetWindowModal(sdlWindow, true);
        }
        else if (!isModal && currentlyModal)
        {
            SDL_SetWindowModal(sdlWindow, false);
        }
    }
};

} // namespace ui::systems