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
#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <entt/entt.hpp>
#include <utils.h>
#include "src/ui/common/Events.h"

namespace ui
{

/**
 * @brief 输入处理任务 - 处理 SDL 事件和 ImGui 输入
 */
struct InputTaskChain final : public entt::process
{
    using allocator_type = typename entt::process::allocator_type;
    using delta_type = typename entt::process::delta_type;

    InputTaskChain(const allocator_type& alloc, delta_type delay)
        : entt::process{alloc}, remainingTime(delay), delayTime(delay)
    {
    }

    void update(const delta_type delta, [[maybe_unused]] void* data) override
    {
        if (remainingTime > delta)
        {
            remainingTime -= delta;
            //    aborted();
            return; // 时间未到，直接返回
        }
        remainingTime = delayTime;
        auto& dispatcher = ::utils::Dispatcher::getInstance();
        dispatcher.enqueue<ui::events::SDLEvent>(ui::events::SDLEvent{});
        remainingTime = delayTime;
    }

    void succeeded() override { LOG_INFO("[input task] InputTask succeeded"); }
    void failed() override { LOG_INFO("[input task] InputTask failed"); }
    void aborted() override { LOG_INFO("[input task] InputTask aborted"); }

private:
    delta_type remainingTime;
    delta_type delayTime;
};

/**
 * @brief 渲染任务 - 执行 ImGui 和 SDL 渲染
 */
struct RenderTaskChain final : public entt::process
{
    using allocator_type = typename entt::process::allocator_type;
    using delta_type = typename entt::process::delta_type;

    RenderTaskChain(const allocator_type& alloc, delta_type delay)
        : entt::process{alloc}, remainingTime(delay), delayTime(delay)
    {
    }

    void update(const delta_type delta, [[maybe_unused]] void* data) override
    {
        if (remainingTime > delta)
        {
            remainingTime -= delta;
            // aborted();
            return;
        }
        remainingTime = delayTime;
        auto& dispatcher = ::utils::Dispatcher::getInstance();
        // 先触发布局更新，再渲染
        dispatcher.trigger<ui::events::UpdateLayout>(ui::events::UpdateLayout{});
        dispatcher.trigger<ui::events::UpdateRendering>(ui::events::UpdateRendering{});

        // succeed();
    }

    void succeeded() override { LOG_INFO("[render task] RenderTask succeeded"); }
    void failed() override { LOG_INFO("[render task] RenderTask failed"); }
    void aborted() override { LOG_INFO("[render task] RenderTask aborted"); }

private:
    delta_type remainingTime;
    delta_type delayTime; // 约60FPS
};

struct EventTaskChain final : public entt::process
{
    using allocator_type = typename entt::process::allocator_type;
    using delta_type = typename entt::process::delta_type;

    EventTaskChain(const allocator_type& alloc, delta_type delay)
        : entt::process{alloc}, remainingTime(delay), delayTime(delay)
    {
    }

    void update(const delta_type delta, [[maybe_unused]] void* data) override
    {
        if (remainingTime > delta)
        {
            remainingTime -= delta;
            // aborted();
            return;
        }
        remainingTime = delayTime;
        auto& dispatcher = ::utils::Dispatcher::getInstance();
        dispatcher.update(); // 处理所有排队的事件
        // succeed();
    }

    void succeeded() override { LOG_INFO("[event task] EventTask succeeded"); }
    void failed() override { LOG_INFO("[event task] EventTask failed"); }
    void aborted() override { LOG_INFO("[event task] EventTask aborted"); }

private:
    delta_type remainingTime;
    delta_type delayTime; // 约60FPS
};

} // namespace ui
