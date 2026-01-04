/**
 * ************************************************************************
 *
 * @file EventLoop.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-19
 * @version 0.1
 * @brief ui事件循环管理类
    基于ASIO实现的跨平台事件循环
    维持ui线程的持续运行
    提供启动和停止事件循环的接口
    ui实体的渲染和输入处理对应的系统被提交到该事件循环中执行
    事件循环本身不管理线程
    1ms间隔轮询事件

    先处理SDL和ImGui的事件，然后驱动ECS系统更新UI状态，然后处理渲染，
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

// 防止 Windows.h 宏污染 ASIO 的 execution 命名空间

#include <asio.hpp>
#include <thread>
#include <memory>
#include <atomic>
#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

namespace ui
{
class EventLoop
{
public:
    EventLoop()
        : m_ioContext(std::make_unique<asio::io_context>()), m_workGuard(asio::make_work_guard(*m_ioContext)),
          m_running(false)
    {
    }

    ~EventLoop() { quit(); }

    void exec()
    {
        m_running.store(true);
        while (m_running.load())
        {
            // 1. 【任务队列部分】
            // 处理所有从其他线程投递过来的 invoke 任务
            // poll() 会执行掉当前队列里所有的任务并立即返回，不会阻塞
            m_ioContext->poll();
            if (m_ioContext->stopped()) m_ioContext->restart();

            // 2. 【默认处理器部分】
            // 如果用户注册了默认处理器，则调用它
            if (m_defaultHandler)
            {
                m_defaultHandler();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    void quit()
    {
        if (!m_running.load()) return;

        m_running.store(false);
        m_workGuard.reset(); // 允许 io_context 停止运行
    }

    // 直接投递一个“零参数可调用对象”（例如带捕获的 lambda）。
    template <typename Func>
        requires std::invocable<std::decay_t<Func>>
    void invoke(Func&& func)
    {
        asio::post(m_ioContext->get_executor(),
                   [fn = std::forward<Func>(func)]() mutable { std::invoke(std::move(fn)); });
    }

    template <typename Func, typename... Args>
        requires(sizeof...(Args) > 0) && std::invocable<std::decay_t<Func>, std::decay_t<Args>...>
    void invoke(Func&& func, Args&&... args)
    {
        asio::post(m_ioContext->get_executor(),
                   [fn = std::forward<Func>(func), ... capturedArgs = std::forward<Args>(args)]() mutable
                   { std::invoke(std::move(fn), std::move(capturedArgs)...); });
    }

    // 注册默认处理器（无参数版本）
    template <typename Func>
        requires std::invocable<std::decay_t<Func>>
    void registerDefaultHandler(Func&& func)
    {
        m_defaultHandler = [fn = std::forward<Func>(func)]() mutable { std::invoke(std::move(fn)); };
    }

    // 注册默认处理器（带参数版本）
    template <typename Func, typename... Args>
        requires(sizeof...(Args) > 0) && std::invocable<std::decay_t<Func>, std::decay_t<Args>...>
    void registerDefaultHandler(Func&& func, Args&&... args)
    {
        m_defaultHandler = [fn = std::forward<Func>(func), ... capturedArgs = std::forward<Args>(args)]() mutable
        { std::invoke(std::move(fn), std::move(capturedArgs)...); };
    }

private:
    std::unique_ptr<asio::io_context> m_ioContext;
    asio::executor_work_guard<asio::io_context::executor_type> m_workGuard;
    std::atomic<bool> m_running;
    std::move_only_function<void()> m_defaultHandler;
};
} // namespace ui