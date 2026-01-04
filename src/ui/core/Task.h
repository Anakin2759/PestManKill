/**
 * ************************************************************************
 *
 * @file Task.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-25
 * @version 0.1
 * @brief  ui每帧执行的任务类
    在system类之上做一层任务封装
  - 定义渲染任务和输入处理任务
  - 任务可由entt::scheduler调度执行
  - 支持任务的初始化、更新、完成和失败回调

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

namespace ui
{

/**
 * @brief 输入处理任务 - 处理 SDL 事件和 ImGui 输入
 */
struct InputTask : public entt::process
{
    using allocator_type = typename entt::process::allocator_type;
    using delta_type = typename entt::process::delta_type;

    InputTask(const allocator_type& alloc, delta_type /*delay*/) : entt::process{alloc} {}

    void update(const delta_type /*delta*/, void* data) override
    {
        auto& dispatcher = ::utils::Dispatcher::getInstance();
        dispatcher.trigger<ui::events::SDLEvent>(ui::events::SDLEvent{});

        // 任务不终止，持续运行
    }

    void init() { LOG_INFO("[input task] InputTask initialized"); }
    void succeeded() { LOG_INFO("[input task] InputTask succeeded"); }
    void failed() { LOG_INFO("[input task] InputTask failed"); }
    void aborted() { LOG_INFO("[input task] InputTask aborted"); }
};

/**
 * @brief 渲染任务 - 执行 ImGui 和 SDL 渲染
 */
struct RenderTask : public entt::process
{
    using allocator_type = typename entt::process::allocator_type;
    using delta_type = typename entt::process::delta_type;

    RenderTask(const allocator_type& alloc, delta_type /*delay*/) : entt::process{alloc} {}

    void update(const delta_type delta, void* data) override {}

    void init() { LOG_INFO("[render task] RenderTask initialized"); }
    void succeeded() { LOG_INFO("[render task] RenderTask succeeded"); }
    void failed() { LOG_INFO("[render task] RenderTask failed"); }
    void aborted() { LOG_INFO("[render task] RenderTask aborted"); }
};

} // namespace ui
