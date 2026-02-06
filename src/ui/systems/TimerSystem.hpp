/**
 * ************************************************************************
 *
 * @file TimerSystem.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-02-03
 * @version 0.1
 * @brief 定时器系统 - 处理定时任务的调度和执行

    - 管理全局定时任务列表
    - 在每帧更新时检查并执行到期任务
    - 检查任务的取消请求
    - 保存任务的执行状态
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <cstdint>
#include <entt/entt.hpp>
#include <vector>
#include "../common/Events.hpp"
#include "../interface/ISystem.hpp"
#include "../singleton/Dispatcher.hpp"
namespace ui::systems
{
/**
 * @brief 定时器系统
 */
class TimerSystem : public interface::EnableRegister<TimerSystem>
{
public:
    /**
     * @brief 注册事件处理器
     */
    void registerHandlersImpl();

    /**
     * @brief 注销事件处理器
     */
    void unregisterHandlersImpl();

    void update() noexcept
    {
        // 遍历所有活动定时器，检查是否到期
        for (auto& timer : m_activeTimers)
        {
        }
    };

    void QuitTimer() noexcept {

    };

    void clearTimer() {

    };

private:
    std::vector<uint32_t> m_activeTimers; // 活动定时器ID列表
};
} // namespace ui::systems