/**
 * ************************************************************************
 *
 * @file TaskChain.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-25
 * @version 0.1
 * @brief  ui每帧执行的任务链封装

  - 定义渲染任务和输入处理任务链
  - 任务链可由entt::scheduler调度执行
  - 支持任务链的初始化、更新、完成和失败回调

  系统类负责具体实现逻辑，任务类负责调度和业务流程
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <entt/entt.hpp>
#include "../common/Events.hpp"
#include "../singleton/Dispatcher.hpp"
#include "../singleton/Logger.hpp"
namespace ui
{

/**
 * @brief 输入处理任务 - 处理 SDL 事件
 */
struct InputTaskChain final : public entt::process
{
    using allocator_type = typename entt::process::allocator_type;
    using delta_type = typename entt::process::delta_type;

    InputTaskChain(const allocator_type& alloc, delta_type delay);

    void update(const delta_type delta, [[maybe_unused]] void* data) override;

    void succeeded() override;
    void failed() override;
    void aborted() override;

private:
    delta_type m_remainingTime;
    delta_type m_delayTime;
};

/**
 * @brief 渲染任务 - 执行SDL 渲染
 */
struct RenderTaskChain final : public entt::process
{
    using allocator_type = typename entt::process::allocator_type;
    using delta_type = typename entt::process::delta_type;

    RenderTaskChain(const allocator_type& alloc, delta_type delay);

    void update(const delta_type delta, [[maybe_unused]] void* data) override;

    void succeeded() override;
    void failed() override;
    void aborted() override;

private:
    delta_type m_remainingTime;
    delta_type m_delayTime; // 约60FPS
};
//
struct EventTaskChain final : public entt::process
{
    using allocator_type = typename entt::process::allocator_type;
    using delta_type = typename entt::process::delta_type;

    EventTaskChain(const allocator_type& alloc, delta_type delay);

    void update(const delta_type delta, [[maybe_unused]] void* data) override;

    void succeeded() override;
    void failed() override;
    void aborted() override;

private:
    delta_type m_remainingTime;
    delta_type m_delayTime; // 约60FPS
};

struct QueuedTaskChain final : public entt::process
{
    using allocator_type = typename entt::process::allocator_type;
    using delta_type = typename entt::process::delta_type;

    explicit QueuedTaskChain(const allocator_type& alloc);

    void update([[maybe_unused]] const delta_type delta, [[maybe_unused]] void* data) override;

    void succeeded() override;
    void failed() override;
    void aborted() override;
};

} // namespace ui
