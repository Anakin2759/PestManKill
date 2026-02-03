#pragma once

#include <cstdint>
#include <entt/entt.hpp>
#include "../common/Types.hpp"
#include "../common/Policies.hpp"
namespace ui::utils
{

bool HasAlignment(policies::Alignment value, policies::Alignment flag);
void SetWindowFlag(::entt::entity entity, policies::WindowFlag flag);

void MarkLayoutDirty(::entt::entity entity);
void MarkRenderDirty(::entt::entity entity);
void CloseWindow(::entt::entity entity);
void QuitUiEventLoop();

void InvokeTask(std::move_only_function<void()> func);
using TaskHandle = uint32_t;
/**
 * @brief 注册一个定时任务，返回任务句柄
 * @param interval 间隔时间（毫秒）
 * @param func 任务函数
 * @return 任务句柄
 */
TaskHandle TimerCallback(uint32_t interval, std::move_only_function<void()> func);
/**
 * @brief 取消注册一个定时任务
 * @param handle 任务句柄
 */
void CancelQueuedTask(TaskHandle handle);
} // namespace ui::utils
