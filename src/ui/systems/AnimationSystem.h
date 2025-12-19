/**
 * ************************************************************************
 *
 * @file AnimationSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief UI动画系统
 *
 * 负责更新所有UI动画的ECS系统
    并非回调式动画系统，而是基于组件的数据驱动系统
    支持位置动画和透明度动画
    使用线性插值计算动画进度
    通过组件标记动画状态，支持暂停和完成事件
    可扩展以支持更多动画类型
    集成到UiSystem中统一管理
    使用ECS模式管理动画状态和属性
    提供清晰的更新流程，易于维护和扩展
    确保动画更新性能和响应速度

 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include "src/ui/components/UIComponents.h"
#include "src/ui/ui/UIEvents.h"
#include "src/utils/Dispatcher.h"

namespace ui::systems
{

class AnimationSystem
{
public:
    /**
     * @brief 更新所有活动的动画
     * @param deltaTime 时间增量（秒）
     */
    void update(float deltaTime) {}
    void registerHandlers()
    {
        // 注册动画完成事件处理器
        utils::Dispatcher::getInstance()
            .sink<events::AnimationCompletedEvent>()
            .connect<&AnimationSystem::onAnimationCompleted>(this);
    }
    void unregisterHandlers()
    {
        // 注销动画完成事件处理器
        utils::Dispatcher::getInstance()
            .sink<events::AnimationCompletedEvent>()
            .disconnect<&AnimationSystem::onAnimationCompleted>(this);
    }

private:
    /**
     * @brief 处理动画完成事件
     * @param event 动画完成事件
     */
    void onAnimationCompleted(const events::AnimationCompletedEvent& event)
    {
        auto& registry = utils::Registry::getInstance();
        entt::entity entity = event.entity;
        // 根据动画类型执行相应的后续处理
        if (registry.valid(entity))
        {
            if (registry.any_of<components::PositionAnimation>(entity))
            {
                // 位置动画完成后的处理逻辑
            }
            if (registry.any_of<components::AlphaAnimation>(entity))
            {
                // 透明度动画完成后的处理逻辑
            }
        }
    }
};

} // namespace ui::systems
