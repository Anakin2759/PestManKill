/**
 * ************************************************************************
 *
 * @file TimerSystem.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-02-03
 * @version 0.1
 * @brief 定时器系统 - 处理定时任务的调度和执行
    取代现在的简单计时器实现，提供更灵活和强大的定时任务管理功能
    - 支持一次性和重复定时任务
    - 支持任务取消和修改
    - 支持任务优先级和分组
    - 提供任务执行状态查询接口

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
#include "../interface/Isystem.hpp"
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

    /**
     * @brief 添加定时任务
     * @param interval 间隔时间（毫秒）
     * @param func 任务函数
     * @param singleShot 是否单次执行（默认为false，重复执行）
     * @return 任务句柄
     */
    static uint32_t addTask(uint32_t interval, std::move_only_function<void()> func, bool singleShot = false);

    /**
     * @brief 取消定时任务
     * @param handle 任务句柄
     */
    static void cancelTask(uint32_t handle);

    /**
     * @brief 更新定时器状态（每帧调用）
     * @param deltaMs 时间增量（毫秒）
     */
    static void update(uint32_t deltaMs);

private:
    void onUpdateTimer(const events::UpdateTimer& event);
};
} // namespace ui::systems