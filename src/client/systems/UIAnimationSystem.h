/**
 * ************************************************************************
 *
 * @file UIAnimationSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief UI动画系统
 *
 * 负责更新所有UI动画的ECS系统
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include "src/client/components/UIComponents.h"
#include "src/client/events/UIEvents.h"
#include "src/client/utils/Dispatcher.h"

namespace ui::systems
{

class UIAnimationSystem
{
public:
    /**
     * @brief 更新所有活动的动画
     * @param deltaTime 时间增量（秒）
     */
    void update(float deltaTime)
    {
        // 更新基础动画
        auto animationView = utils::Registry::getInstance().view<components::Animation>();
        for (auto entity : animationView)
        {
            auto& animation = animationView.get<components::Animation>(entity);
            if (!animation.active)
            {
                continue;
            }

            animation.elapsed += deltaTime;
            float progress = std::min(1.0F, animation.elapsed / animation.duration);

            if (animation.updateCallback)
            {
                animation.updateCallback(progress);
            }

            if (progress >= 1.0F)
            {
                animation.active = false;
                utils::Dispatcher::getInstance().enqueue<events::AnimationComplete>(entity);
            }
        }

        // 更新位置动画
        auto posAnimView = utils::Registry::getInstance()
                               .view<components::PositionAnimation, components::Position, components::Animation>();
        for (auto entity : posAnimView)
        {
            const auto& animation = posAnimView.get<components::Animation>(entity);
            if (!animation.active)
            {
                continue;
            }

            auto& posAnim = posAnimView.get<components::PositionAnimation>(entity);
            auto& position = posAnimView.get<components::Position>(entity);

            float progress = animation.elapsed / animation.duration;
            progress = std::min(1.0F, progress);
            posAnim.progress = progress;

            // 线性插值
            position.x = posAnim.startPos.x + (posAnim.endPos.x - posAnim.startPos.x) * progress;
            position.y = posAnim.startPos.y + (posAnim.endPos.y - posAnim.startPos.y) * progress;
        }

        // 更新透明度动画
        auto alphaAnimView = utils::Registry::getInstance()
                                 .view<components::AlphaAnimation, components::Visibility, components::Animation>();
        for (auto entity : alphaAnimView)
        {
            const auto& animation = alphaAnimView.get<components::Animation>(entity);
            if (!animation.active)
            {
                continue;
            }

            auto& alphaAnim = alphaAnimView.get<components::AlphaAnimation>(entity);
            auto& visibility = alphaAnimView.get<components::Visibility>(entity);

            float progress = animation.elapsed / animation.duration;
            progress = std::min(1.0F, progress);
            alphaAnim.progress = progress;

            // 线性插值
            visibility.alpha = alphaAnim.startAlpha + (alphaAnim.endAlpha - alphaAnim.startAlpha) * progress;
        }
    }
};

} // namespace ui::systems
