/**
 * ************************************************************************
 *
 * @file AnimationSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief UI动画系统
 *
 * 负责更新所有UI动画的ECS系统，事件驱动。
    不负责渲染，只更新动画状态
    基于组件的数据驱动系统
    支持位置动画和透明度动画
    使用线性插值计算动画进度
    通过组件标记动画状态，支持暂停和完成事件
    使用ECS模式管理动画状态和属性
    提供清晰的更新流程，易于维护和扩展
    确保动画更新性能和响应速度
    在渲染前更新动画状态，确保视觉一致性

 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include "src/ui/components/Components.h"
#include "src/ui/components/Events.h"
#include "src/utils/Dispatcher.h"
#include "interface/Isystem.h"
#include <sys/stat.h>
#include <utils.h>
namespace ui::systems
{

class AnimationSystem : public ui::interface::EnableRegister<AnimationSystem>
{
    static constexpr float DELTA_TIME = 0.016F; // 约 60 FPS，实际应该从 SystemManager 传入

public:
    void registerHandlersImpl() {}
    void unregisterHandlersImpl() {}

private:
    /**
     * @brief 更新所有活动的动画
     */
    void update()
    {
        auto& registry = utils::Registry::getInstance();
        auto& dispatcher = utils::Dispatcher::getInstance();

        // 获取所有具有动画时间组件的实体
        auto view = registry.view<components::AnimationTime>();

        for (auto entity : view)
        {
            auto& animTime = view.get<components::AnimationTime>(entity);

            // 更新已消逝时间（假设固定 deltaTime，实际应从参数传入）
            // 注意：这里需要从外部传入 deltaTime
            float deltaTime = DELTA_TIME;
            animTime.elapsed += deltaTime;

            // 计算动画进度 [0, 1]
            float progress = std::min(animTime.elapsed / animTime.duration, 1.0f);

            // 应用缓动函数
            float easedProgress = applyEasing(progress, animTime.easing);

            // 更新位置动画
            if (auto* animPos = registry.try_get<components::AnimationPosition>(entity))
            {
                if (auto* pos = registry.try_get<components::Position>(entity))
                {
                    pos->value.x = animPos->from.x + (animPos->to.x - animPos->from.x) * easedProgress;
                    pos->value.y = animPos->from.y + (animPos->to.y - animPos->from.y) * easedProgress;
                }
            }

            // 更新透明度动画
            if (auto* animAlpha = registry.try_get<components::AnimationAlpha>(entity))
            {
                auto& alpha = registry.get_or_emplace<components::Alpha>(entity);
                alpha.value = animAlpha->from + (animAlpha->to - animAlpha->from) * easedProgress;
            }

            // 检查动画是否完成
            if (animTime.elapsed >= animTime.duration)
            {
                // 根据播放模式处理
                if (animTime.mode == policies::Play::ONCE)
                {
                    // 移除动画组件
                    registry.remove<components::AnimationTime>(entity);
                    if (registry.all_of<components::AnimationPosition>(entity))
                    {
                        registry.remove<components::AnimationPosition>(entity);
                    }
                    if (registry.all_of<components::AnimationAlpha>(entity))
                    {
                        registry.remove<components::AnimationAlpha>(entity);
                    }

                    // 触发动画完成事件
                    dispatcher.enqueue<events::AnimationComplete>(events::AnimationComplete{entity});
                }
                else if (animTime.mode == policies::Play::LOOP)
                {
                    // 重置动画
                    animTime.elapsed = 0.0F;
                }
            }
        }
    }
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
