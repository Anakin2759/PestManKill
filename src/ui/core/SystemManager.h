/**
 * ************************************************************************
 *
 * @file SystemManager.h
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
#include "src/ui/systems/RenderSystem.h"
#include "src/ui/systems/AnimationSystem.h"
#include "src/ui/systems/InteractionSystem.h"
#include "src/ui/systems/LayoutSystem.h"
#include "src/ui/systems/WindowsSystem.h" // 保持与 Application.h 中的一致
// 引入其他依赖
#include "src/ui/core/Factory.h"
#include "src/ui/components/Components.h"
#include "src/ui/components/Events.h"
#include <utils.h>

namespace ui
{

/**
 * @brief UI系统管理器：定义ECS系统的执行流程
 */
class SystemManager
{
public:
    // 构造函数：初始化所有子系统
    SystemManager() {
        
    };

    ~SystemManager() = default;

    // 禁用拷贝和移动
    SystemManager(const SystemManager&) = delete;
    SystemManager& operator=(const SystemManager&) = delete;
    SystemManager(SystemManager&&) = delete;
    SystemManager& operator=(SystemManager&&) = delete;

    

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

    [[nodiscard]] systems::RenderSystem& getRenderSystem() { return m_renderSystem; }
    [[nodiscard]] systems::AnimationSystem& getAnimationSystem() { return m_animationSystem; }
    [[nodiscard]] systems::InteractionSystem& getInteractionSystem() { return m_interactionSystem; }
    [[nodiscard]] systems::LayoutSystem& getLayoutSystem() { return m_layoutSystem; }

    /**
     * @brief 清空所有UI元素（清空整个注册表）
     */
    void clear() { utils::Registry::getInstance().clear(); }

private:
    // 声明所有子系统实例
    systems::InteractionSystem m_interactionSystem; // 1. 输入/交互
    systems::AnimationSystem m_animationSystem;     // 2. 动画/状态更新
    systems::LayoutSystem m_layoutSystem;           // 3. 布局计算
    systems::RenderSystem m_renderSystem;           // 4. 渲染绘制
    systems::WindowSystem m_windowSystem;           // 窗口管理系统
};
} // namespace ui