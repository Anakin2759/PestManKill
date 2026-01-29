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

// 引入系统接口
#include "../interface/Isystem.hpp"

namespace ui
{

/**
 * @brief UI系统管理器：定义ECS系统的执行流程
 * 使用 entt::poly 实现系统的动态管理
 */
class SystemManager
{
private:
    // 使用 entt::poly 动态管理所有系统
    std::vector<entt::poly<interface::ISystem>> m_systems;

    // 系统索引（用于快速访问特定系统）
    enum SystemIndex : uint8_t
    {
        INTERACTION = 0, // 鼠标交互系统
        HITTEST = 1,     // 命中测试系统
        ANIMATION = 2,   // 动画系统
        LAYOUT = 3,      // 布局系统
        RENDER = 4,      // 渲染系统
        WINDOW = 5,      // 窗口同步系统
        ACTION = 6
    };
    static constexpr size_t SYSTEM_COUNT = 7;

public:
    // 构造函数：初始化所有子系统
    SystemManager();

    ~SystemManager();

    // 禁用拷贝和移动
    SystemManager(const SystemManager&) = delete;
    SystemManager& operator=(const SystemManager&) = delete;
    SystemManager(SystemManager&&) = delete;
    SystemManager& operator=(SystemManager&&) = delete;

    /**
     * @brief 注册所有系统的事件处理器
     */
    void registerAllHandlers();

    /**
     * @brief 注销所有系统的事件处理器
     */
    void unregisterAllHandlers();

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
    void removeSystem(uint8_t index);
    /**
     * @brief 获取系统数量
     */
    [[nodiscard]] size_t getSystemCount() const { return m_systems.size(); }

    /**
     * @brief 清空所有UI元素 携带uitag的组件
     */
    void clear(); // NOLINT(readability-convert-member-functions-to-static)
};
} // namespace ui