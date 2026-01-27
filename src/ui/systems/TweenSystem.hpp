/**
 * ************************************************************************
 *
 * @file TweenSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief UI动画系统
 *
 * 负责更新所有UI动画的ECS系统，事件驱动。
    不负责渲染，只更新动画状态

    在布局和渲染系统之前运行
    基于组件的数据驱动系统

    只做插值计算和状态更新

 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include "../common/Components.hpp"
#include "../common/Events.hpp"
#include "../singleton/Dispatcher.hpp"
#include "../singleton/Registry.hpp"
#include "../interface/Isystem.hpp"
#include <sys/stat.h>

namespace ui::systems
{

class TweenSystem : public ui::interface::EnableRegister<TweenSystem>
{
    static constexpr float DELTA_TIME = 0.016F; // 约 60 FPS，实际应该从 SystemManager 传入

public:
    void registerHandlersImpl() {}
    void unregisterHandlersImpl() {}

private:
    /**
     * @brief 更新所有活动的动画
     */
    void update() {}
    /**
     * @brief 应用缓动函数
     */
    static float applyEasing(float time, policies::Easing easing)
    {
        switch (easing)
        {
            case policies::Easing::Linear: // 线性缓动
                return time;
            case policies::Easing::EaseInQuad: // 二次缓入
                return time * time;
            case policies::Easing::EaseOutQuad: // 二次缓出
                return time * (2.0F - time);
            case policies::Easing::EaseInOutQuad: // 二次缓入缓出
                return time < 0.5F ? 2.0F * time * time : -1.0F + (4.0F - 2.0F * time) * time;
            default:
                return time;
        }
    }
};

} // namespace ui::systems
