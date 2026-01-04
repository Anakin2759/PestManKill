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
#include "systems/WindowsSystem.h"
#include "components/Events.h"
#include "Task.h"
namespace ui
{
class Application
{
public:
    /**
     * @brief 构造函数：初始化所有外部和内部资源
     * @param title 窗口标题
     * @param width 窗口宽度
     * @param height 窗口高度
     */
    explicit Application(const char* title = "PestManKill UI", int width = 800, int height = 600)
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

        // 并行调度输入和渲染任务
        m_scheduler.attach<ui::InputTask>(16u);
        m_scheduler.attach<ui::RenderTask>(16u);

        m_eventLoop.registerDefaultHandler(
            [this]()
            {
                // 传递渲染上下文给任务
                m_scheduler.update(16u);
            });

        auto& dispatcher = utils::Dispatcher::getInstance();
        dispatcher.sink<ui::events::QuitRequested>().connect<&Application::onQuitRequested>(*this);
    }

    void onQuitRequested(ui::events::QuitRequested&) { m_eventLoop.quit(); }

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
    // 图形上下文（包含 SDL 窗口和渲染器）
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

    // 主循环控制
    bool m_running = true;

    // 时间管理
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    TimePoint m_lastTime = Clock::now();

    // 阻止拷贝和移动（通常 Application 是单例或独占资源）
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;
};
} // namespace ui