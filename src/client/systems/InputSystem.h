/**
 * ************************************************************************
 *
 * @file InputSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-08
 * @version 0.1
 * @brief 输入捕获处理系统
  处理全局输入事件，如键盘和鼠标输入
  将输入事件分发到相应的UI组件进行处理
  使用ECS模式，监听输入相关组件并执行逻辑
  暂时支持鼠标事件
  鼠标事件 层层判断点击位置/可见性/启用状态等

  
  集成ImGui输入处理，确保UI交互流畅
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include "src/client/components/InputComponents.h"
#include "src/client/utils/utils.h"
namespace ui::systems
{
class InputSystem
{
public:
    /**
     * @brief 处理输入事件
     */
    void update() noexcept
    {
        
    }
};

} // namespace ui::systems