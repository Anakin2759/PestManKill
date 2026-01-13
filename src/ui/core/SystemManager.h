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
#include <cstddef>
#include <cstdint>
#include <entt/entt.hpp>
#include <SDL3/SDL.h>
#include <imgui.h>

// 引入图形上下文
#include "GraphicsContext.h"

// 引入系统接口
#include "src/ui/interface/Isystem.h"

// 引入所有子系统头文件
#include "src/ui/systems/RenderSystem.h"
#include "src/ui/systems/AnimationSystem.h"
#include "src/ui/systems/InteractionSystem.h"
#include "src/ui/systems/LayoutSystem.h"
#include "src/ui/systems/WindowsSystem.h" // 保持与 Application.h 中的一致
#include "src/ui/systems/ActionSystem.h"
#include "src/ui/common/Events.h"
#include <utils.h>

namespace ui::events
{
struct GraphicsContextSetEvent;
}

namespace ui
{

/**
 * @brief UI系统管理器：定义ECS系统的执行流程
 * 使用 entt::poly 实现系统的动态管理
 */
class SystemManager
{
private:
    // GraphicsContext 引用（由 Application 注入）
    GraphicsContext* m_graphicsContext = nullptr;

    // 使用 entt::poly 动态管理所有系统
    std::vector<entt::poly<interface::ISystem>> m_systems;

    // 系统索引（用于快速访问特定系统）
    enum SystemIndex : uint8_t
    {
        INTERACTION = 0, // 鼠标交互系统
        ANIMATION = 1,   // 动画系统
        LAYOUT = 2,      // 布局系统
        RENDER = 3,      // 渲染系统
        WINDOW = 4,      // 窗口同步系统
        ACTION = 5
    };
    static constexpr size_t SYSTEM_COUNT = 6;

public:
    // 构造函数：初始化所有子系统
    SystemManager()
    {
        // 预留空间
        m_systems.reserve(SYSTEM_COUNT);

        // 按顺序添加系统
        m_systems.emplace_back(systems::InteractionSystem{});
        m_systems.emplace_back(systems::AnimationSystem{});
        m_systems.emplace_back(systems::LayoutSystem{});
        m_systems.emplace_back(systems::RenderSystem{});
        m_systems.emplace_back(systems::WindowSystem{});
        m_systems.emplace_back(systems::ActionSystem{});
    }

    ~SystemManager() { unregisterAllHandlers(); };

    // 禁用拷贝和移动
    SystemManager(const SystemManager&) = delete;
    SystemManager& operator=(const SystemManager&) = delete;
    SystemManager(SystemManager&&) = delete;
    SystemManager& operator=(SystemManager&&) = delete;

    /**
     * @brief setGraphicsContext 设置图形上下文
     * @param context GraphicsContext* 图形上下文指针
     */
    void setGraphicsContext(GraphicsContext* context)
    {
        m_graphicsContext = context;
        // 通过事件分发器发布上下文设置事件
        auto& dispatcher = utils::Dispatcher::getInstance();
        dispatcher.enqueue<events::GraphicsContextSetEvent>(context);
    }

    /**
     * @brief 注册所有系统的事件处理器
     */
    void registerAllHandlers()
    {
        for (auto& system : m_systems)
        {
            system->registerHandlers();
        }
    }

    /**
     * @brief 注销所有系统的事件处理器
     */
    void unregisterAllHandlers()
    {
        for (auto& system : m_systems)
        {
            system->unregisterHandlers();
        }
    }

    /**
     * @brief 动态添加系统
     * @tparam T 系统类型
     * @param system 系统实例
     */
    template <typename T>
    void addSystem(T&& system)
    {
        m_systems.emplace_back(std::forward<T>(system));
    }

    /**
     * @brief 移除指定索引的系统
     * @param index 系统索引
     */
    void removeSystem(uint8_t index)
    {
        if (index < m_systems.size())
        {
            m_systems.erase(m_systems.begin() + index);
        }
    }
    /**
     * @brief 获取系统数量
     */
    [[nodiscard]] size_t getSystemCount() const { return m_systems.size(); }

    /**
     * @brief 清空所有UI元素 携带uitag的组件
     */
    void clear()
    {
        auto& registry = utils::Registry::getInstance();
        auto view = registry.view<components::UiTag>();
        registry.destroy(view.begin(), view.end());
    }
};
} // namespace ui