/**
 * ************************************************************************
 *
 * @file UiSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief UI系统管理器 - 基于ECS架构
 *
 * 管理UI渲染系统、动画系统和事件系统
 * 使用EnTT库实现完全ECS化的UI框架
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include <SDL3/SDL.h>
#include <imgui.h>
#include <memory>
#include "src/client/systems/UIRenderSystem.h"
#include "src/client/systems/UIAnimationSystem.h"
#include "src/client/model/UIFactory.h"
#include "src/client/utils/Dispatcher.h"
#include "src/client/components/UIComponents.h"
#include "src/client/events/UIEvents.h"
#include "src/client/utils/utils.h"
namespace ui
{

/**
 * @brief UI系统管理器
 *
 * 负责管理所有UI相关的ECS系统
 * 提供统一的更新和渲染接口
 */
class UiSystem
{
public:
    UiSystem() { setupEventHandlers(); }

    ~UiSystem() = default;

    UiSystem(const UiSystem&) = delete;
    UiSystem& operator=(const UiSystem&) = delete;
    UiSystem(UiSystem&&) = delete;
    UiSystem& operator=(UiSystem&&) = delete;

    /**
     * @brief 更新UI系统
     * @param deltaTime 时间增量（秒）
     */
    void update(float deltaTime)
    {
        // 更新动画系统
        m_animationSystem.update(deltaTime);

        // 处理事件队列
        utils::Dispatcher::getInstance().update();
    }

    /**
     * @brief 渲染所有UI元素
     */
    void render() { m_renderSystem.update(); }

    /**
     * @brief 获取UI工厂
     */
    [[nodiscard]] UIFactory& getFactory() { return m_factory; }
    [[nodiscard]] const UIFactory& getFactory() const { return m_factory; }

    /**
     * @brief 获取渲染系统
     */
    [[nodiscard]] systems::UIRenderSystem& getRenderSystem() { return m_renderSystem; }
    [[nodiscard]] const systems::UIRenderSystem& getRenderSystem() const { return m_renderSystem; }

    /**
     * @brief 获取动画系统
     */
    [[nodiscard]] systems::UIAnimationSystem& getAnimationSystem() { return m_animationSystem; }
    [[nodiscard]] const systems::UIAnimationSystem& getAnimationSystem() const { return m_animationSystem; }

    /**
     * @brief 清空所有UI元素
     */
    void clear() { utils::Registry::getInstance().clear(); }

private:
    /**
     * @brief 设置事件处理器
     */
    void setupEventHandlers()
    {
        // 可以在这里注册全局事件监听器
        // 例如：utils::Dispatcher::getInstance().sink<events::ButtonClicked>().connect<&UiSystem::onButtonClicked>(this);
    }

private:
    systems::UIRenderSystem m_renderSystem;
    systems::UIAnimationSystem m_animationSystem;

    UIFactory m_factory;
};
} // namespace ui
