/**
 * ************************************************************************
 *
 * @file UiSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-11 (Updated)
 * @version 0.2
 * @brief UI系统管理器 - 基于ECS架构
 *
 * 负责管理所有UI相关的ECS系统，并按正确的顺序调用它们的更新流程。
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

// 引入所有子系统头文件
#include "src/client/systems/UIRenderSystem.h"
#include "src/client/systems/UIAnimationSystem.h"
#include "src/client/systems/InteractionSystem.h"
#include "src/client/systems/UILayoutSystem.h"
#include "src/client/systems/WindowSystem.h" // 保持与 Application.h 中的一致

// 引入其他依赖
#include "src/client/model/UIFactory.h"
#include "src/client/utils/Dispatcher.h"
#include "src/client/components/UIComponents.h"
#include "src/client/events/UIEvents.h"
#include "src/client/utils/utils.h"

namespace ui
{

/**
 * @brief UI系统管理器：定义ECS系统的执行流程
 */
class UiSystem
{
public:
    // 构造函数：初始化所有子系统
    UiSystem() { setupEventHandlers(); }

    ~UiSystem() = default;

    // 禁用拷贝和移动
    UiSystem(const UiSystem&) = delete;
    UiSystem& operator=(const UiSystem&) = delete;
    UiSystem(UiSystem&&) = delete;
    UiSystem& operator=(UiSystem&&) = delete;

    /**
     * @brief 更新UI系统：按固定流程顺序执行所有System
     * @param deltaTime 时间增量（秒）
     *
     */
    void update(float deltaTime)
    {
        // ----------------------------------------------------
        // 1. 输入/交互：将原始输入映射为 ECS 状态 (HoveredTag, ActiveTag)
        // 必须最先运行
        m_interactionSystem.update();

        // ----------------------------------------------------
        // 2. 动画：根据时间更新实体的位置、尺寸、透明度等组件
        // 运行后可能触发 LayoutDirtyTag
        m_animationSystem.update(deltaTime);

        // ----------------------------------------------------
        // 3. 布局：计算并设置 Position 和 Size 组件
        // 必须在所有可能修改尺寸或位置的系统（如动画）之后运行
        m_layoutSystem.update();

        // ----------------------------------------------------
        // 4. 事件处理：处理所有系统和用户触发的事件
        utils::Dispatcher::getInstance().update();
    }

    /**
     * @brief 渲染所有UI元素
     * @param renderer SDL渲染器指针
     */
    void render(SDL_Renderer* renderer)
    {
        // 渲染系统必须在布局系统之后运行，以获取最终的屏幕位置和尺寸
        m_renderSystem.update(renderer);
    }

    // =======================================================
    // Getter 保持不变，但为了简洁，只保留主要的。
    // =======================================================

    [[nodiscard]] systems::UIRenderSystem& getRenderSystem() { return m_renderSystem; }
    [[nodiscard]] systems::UIAnimationSystem& getAnimationSystem() { return m_animationSystem; }
    [[nodiscard]] systems::InteractionSystem& getInteractionSystem() { return m_interactionSystem; }
    [[nodiscard]] systems::UILayoutSystem& getLayoutSystem() { return m_layoutSystem; }

    /**
     * @brief 清空所有UI元素（清空整个注册表）
     */
    void clear() { utils::Registry::getInstance().clear(); }

private:
    /**
     * @brief 设置事件处理器
     */
    void setupEventHandlers()
    {
        // 示例：在这里注册系统级别的事件处理
        // 比如：监听 WindowResizeEvent 并将其转发给 WindowSystem
    }

private:
    // 声明所有子系统实例
    systems::InteractionSystem m_interactionSystem; // 1. 输入/交互
    systems::UIAnimationSystem m_animationSystem;   // 2. 动画/状态更新
    systems::UILayoutSystem m_layoutSystem;         // 3. 布局计算
    systems::UIRenderSystem m_renderSystem;         // 4. 渲染绘制
    // systems::WindowSystem m_windowSystem; // WindowSystem 通常放在 Application 或专门的初始化层
};
} // namespace ui