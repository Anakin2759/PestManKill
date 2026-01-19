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
#include <stdexcept>
#include <chrono>
#include <utils.h>
#include "SystemManager.h"
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
    explicit Application(int argc, char* argv[])
    {
        // 1. 初始化 SDL 子系统（必须在 GraphicsContext 之前）
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        {
            throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
        }

        // 3. 注册系统事件处理器
        m_systems.registerAllHandlers();

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
        m_eventLoop.quit();
    }

    virtual ~Application()
    {
        // 先注销系统，确保 GPU 资源与窗口正确释放
        m_systems.unregisterAllHandlers();

        SDL_Quit();
    }

    /**
     * @brief 应用主循环
     */
    void exec() { m_eventLoop.exec(); }

private:
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