/**
 * ************************************************************************
 *
 * @file Application.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-11 (Updated)
 * @version 0.2
 * @brief ui上下文管理类
 *
 * 负责主循环、输入事件处理、图形上下文管理以及驱动所有ECS系统。
    存在一个单例指针，方便全局访问。
    不是根实体，也不管理根实体
    只负责驱动UiSystem和处理SDL/ImGui集成
    不负责具体UI逻辑和组件管理
    提供初始化和清理接口
    提供运行主循环的接口
    提供访问根实体的接口
    提供访问图形上下文的接口
    一切修改UI状态的操作均通过ECS系统函数在eventloop中完成，

  - exec 方法启动主循环

 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <stdexcept>
#include <chrono>
#include <utils.h>
#include "SystemManager.h"
#include "GraphicsContext.h"
#include "ImguiContext.h"
#include "EventLoop.h"
#include "TaskChain.h"
#include "ThreadPool.h"
#include "common/Events.h"
#include "src/ui/common/Components.h"
#include "src/ui/common/Tags.h"
namespace ui
{
class Application
{
    static constexpr int DEFAULT_WIDTH = 800;
    static constexpr int DEFAULT_HEIGHT = 600;
    static constexpr int FRAME_DELAY_MS = 16;     // ~60 FPS
    static constexpr int MAX_FRAME_TIME_MS = 250; // 防止卡顿时长时间更新
    static constexpr int LOOP_DELAY_MS = 1;       // 主循环延迟，防止100% CPU占用

public:
    /**
     * @brief 构造函数：初始化所有外部和内部资源
     * @param title 窗口标题
     * @param width 窗口宽度
     * @param height 窗口高度
     */
    explicit Application(const char* title = "PestManKill UI", int width = DEFAULT_WIDTH, int height = DEFAULT_HEIGHT)
    {
        // 1. 初始化 SDL 子系统（必须在 GraphicsContext 之前）
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        {
            throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
        }
        // 2. 初始化图形上下文
        m_graphicsContext = std::make_unique<GraphicsContext>(title, width, height);
        // 3. 初始化 ImGui 上下文
        m_imguiContext =
            std::make_unique<ImguiContext>(m_graphicsContext->getWindow(), m_graphicsContext->getRenderer());
        m_systems.registerAllHandlers();

        // 4. 设置图形上下文到系统管理器（必须在 registerAllHandlers 之后）
        m_systems.setGraphicsContext(m_graphicsContext.get());

        // 立即处理排队的事件，确保 GraphicsContext 被分发
        utils::Dispatcher::getInstance().update();

        // 5. 初始化 ECS 主画布实体（MainWidgetTag）。
        // LayoutSystem 只依赖 Position/Size：画布尺寸由该实体的 Size 提供。
        {
            auto& registry = utils::Registry::getInstance();
            bool hasMainWidget = false;
            auto view = registry.view<ui::components::MainWidgetTag>();
            hasMainWidget = (view.begin() != view.end());

            if (!hasMainWidget)
            {
                m_rootEntity = registry.create();
                auto& baseInfo = registry.emplace<ui::components::BaseInfo>(m_rootEntity);
                baseInfo.alias = "rootCanvas";

                registry.emplace<ui::components::UiTag>(m_rootEntity);
                registry.emplace<ui::components::MainWidgetTag>(m_rootEntity);
                registry.emplace<ui::components::VisibleTag>(m_rootEntity);
                registry.emplace<ui::components::Hierarchy>(m_rootEntity);

                auto& pos = registry.emplace<ui::components::Position>(m_rootEntity);
                pos.value = {0.0F, 0.0F};

                auto& size = registry.emplace<ui::components::Size>(m_rootEntity);
                size.autoSize = false;
                size.size = {static_cast<float>(width), static_cast<float>(height)};
                size.widthPolicy = ui::policies::Size::Fixed;
                size.heightPolicy = ui::policies::Size::Fixed;

                registry.emplace_or_replace<ui::components::LayoutDirtyTag>(m_rootEntity);
            }
        }

        // 顺序执行事件缓冲处理->输入->渲染任务
        m_scheduler.attach<ui::QueuedTaskChain>();
        m_scheduler.attach<ui::EventTaskChain>(FRAME_DELAY_MS);
        m_scheduler.attach<ui::InputTaskChain>(FRAME_DELAY_MS);
        m_scheduler.attach<ui::RenderTaskChain>(FRAME_DELAY_MS);

        m_eventLoop.registerDefaultHandler(
            [this]()
            {
                // 2. 执行任务调度（输入/渲染）
                m_scheduler.update(LOOP_DELAY_MS);
                SDL_Delay(LOOP_DELAY_MS); // 避免100% CPU占用
            });

        auto& dispatcher = utils::Dispatcher::getInstance();
        dispatcher.sink<ui::events::QuitRequested>().connect<&Application::onQuitRequested>(*this);
    }
    // 阻止拷贝和移动（通常 Application 是独占资源）
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    void onQuitRequested([[maybe_unused]] ui::events::QuitRequested& event)
    {
        m_scheduler.update(LOOP_DELAY_MS);
        utils::ThreadPool::enqueue([this]() { m_eventLoop.quit(); });
        // m_eventLoop.quit();
    }

    virtual ~Application()
    {
        m_imguiContext.reset();

        SDL_Quit();
    }

    /**
     * @brief 应用主循环
     */
    void exec() { m_eventLoop.exec(); }

private:
    std::unique_ptr<GraphicsContext> m_graphicsContext;

    // ImGui 上下文管理
    std::unique_ptr<ImguiContext> m_imguiContext;

    // 事件循环
    EventLoop m_eventLoop;

    entt::scheduler m_scheduler;

    // 核心 ECS 系统封装
    SystemManager m_systems;

    // ECS 根实体，代表整个屏幕/应用区域
    entt::entity m_rootEntity = entt::null;

    // 时间管理
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    TimePoint m_lastTime = Clock::now();
};
} // namespace ui