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
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <asio.hpp>
#include <thread>
#include <memory>
#include <atomic>

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
        if (m_running.load()) return;

        m_running.store(true);
        m_ioContext->run();
    }

    void quit()
    {
        if (!m_running.load()) return;

        m_running.store(false);
        m_workGuard.reset(); // 允许 io_context 停止运行
    }

    asio::io_context& getContext() { return *m_ioContext; }

private:
    std::unique_ptr<asio::io_context> m_ioContext;
    asio::executor_work_guard<asio::io_context::executor_type> m_workGuard;
    std::atomic<bool> m_running;
};
} // namespace ui