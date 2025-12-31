/**
 * ************************************************************************
 *
 * @file Task.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-25
 * @version 0.1
 * @brief  ui每帧执行的任务类
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include <utils.h>
/**
 * @brief 渲染任务
 */
struct RenderTask : public entt::process<RenderTask, std::uint32_t>
{
    using delta_type = std::uint32_t;

    RenderTask(delta_type delay) : remaining{delay} {}

    void update(delta_type delta, void*) { LOG_INFO("[render task]RenderTask update, delta: " << delta << std::endl); }

    void init() { LOG_INFO("[render task]RenderTask init" << std::endl); }
    void succeeded() { LOG_INFO("[render task]RenderTask succeeded" << std::endl); }
    void failed() { LOG_INFO("[render task]RenderTask failed" << std::endl); }
    void aborted() { LOG_INFO("[render task]RenderTask aborted" << std::endl); }

private:
    uint32_t remaining = 16; // 默认16ms帧间隔
};

/**
 * @brief 输入处理任务 在渲染任务之前执行
 */
struct InputTask : public entt::process<Task, std::uint32_t>
{
    void update(std::uint32_t delta, void*)
    {
        // 处理输入逻辑
        LOG_INFO("[input task]InputTask update, delta: " << delta << std::endl);
    }
    void init() { LOG_INFO("[input task]InputTask init" << std::endl); }
    void succeeded() { LOG_INFO("[input task]InputTask succeeded" << std::endl); }
    void failed() { LOG_INFO("[input task]InputTask failed" << std::endl); }
    void aborted() { LOG_INFO("[input task]InputTask aborted" << std::endl); }

private:
    uint32_t remaining = 16; // 默认16ms帧间隔
};