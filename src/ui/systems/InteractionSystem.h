/**
 * ************************************************************************
 *
 * @file InteractionSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-11 (Updated)
 * @version 0.2
 * @brief 交互处理系统
 *
 * 将原始输入事件映射到UI实体的交互组件上，并更新 Hover/Active/Dirty 等 ECS 状态。
 * 负责处理点击、悬停等交互逻辑，触发相应的UI事件和通知后台。
    从SDL或ImGui获取鼠标位置和按键状态。
    遍历所有可交互实体，执行点碰撞测试 (Hit Test)。
    更新实体的 HoveredTag 和 ActiveTag 组件。
    处理点击事件，触发 ButtonClickedEvent 等UI事件。
    管理交互状态，确保正确的状态转换和事件触发。
    优化交互检测顺序，支持Z-Order排序。
    易于扩展以支持更多交互类型和复杂逻辑。
 *
 * ************************************************************************
 */
#pragma once
#include <entt/entt.hpp>
#include <algorithm>
#include <functional>
#include "src/utils/Registry.h"             // 包含 Registry
#include "src/utils/Dispatcher.h"           // 包含 Dispatcher
#include "src/ui/components/UIComponents.h" // 包含 Position, Size, Clickable, ButtonState, Hierarchy
#include "src/ui/components/UITags.h"       // 包含 HoveredTag, ActiveTag, DisabledTag, LayoutDirtyTag
#include "src/ui/ui/UIEvents.h"             // 包含 ButtonClickedEvent

namespace ui::systems
{
// 假设 utils::Input::getMousePosition() 和 utils::Input::isMouseButtonDown() 已存在
namespace input_utils
{
struct Input
{
    static ImVec2 getMousePosition() { return ImGui::GetIO().MousePos; }
    static bool isMouseButtonDown(int button) { return ImGui::GetIO().MouseDown[button]; }
    static bool isMouseButtonClicked(int button) { return ImGui::IsMouseClicked(button); }
};
} // namespace input_utils

class InteractionSystem
{
private:
    entt::entity m_activeEntity = entt::null; // 当前处于 Active (鼠标按下) 状态的实体

    /**
     * @brief 执行点碰撞测试 (Hit Test)
     * @param point 鼠标绝对位置
     * @param pos 实体绝对位置
     * @param size 实体尺寸
     * @return bool 是否命中
     */
    bool isPointInRect(const ImVec2& point, const ImVec2& pos, const components::Size& size) const
    {
        return point.x >= pos.x && point.x < (pos.x + size.width) && point.y >= pos.y &&
               point.y < (pos.y + size.height);
    }

    /**
     * @brief 计算实体的绝对位置
     * @param registry 注册表
     * @param entity 当前实体
     * @return ImVec2 实体的绝对位置
     */
    ImVec2 getAbsolutePosition(entt::registry& registry, entt::entity entity)
    {
        ImVec2 pos(0.0f, 0.0f);
        entt::entity current = entity;

        // 遍历层级，累加相对位置
        while (current != entt::null && registry.valid(current))
        {
            const auto* posComp = registry.try_get<components::Position>(current);
            if (posComp)
            {
                pos.x += posComp->x;
                pos.y += posComp->y;
            }
            const auto* hierarchy = registry.try_get<components::Hierarchy>(current);
            current = hierarchy ? hierarchy->parent : entt::null;
        }
        return pos;
    }

    /**
     * @brief 获取按 Z-Order 从前到后排序的可交互实体列表
     * @note 实际项目中，UI 实体应该根据 Z-Index Component 或渲染顺序预排序。
     * 此处为简化，仅按 Hierarchy 深度排序（深度越大越靠前）
     */
    std::vector<entt::entity> getZOrderedInteractables(entt::registry& registry)
    {
        std::vector<std::pair<int, entt::entity>> interactables;
        auto view = registry.view<const components::Position, const components::Size, const components::Clickable>();

        for (auto entity : view)
        {
            if (registry.any_of<components::DisabledTag>(entity)) continue; // 忽略禁用的实体

            // 简单深度排序：层级越深（子元素），Z-Order 越高
            int depth = 0;
            entt::entity current = entity;
            while (current != entt::null)
            {
                const auto* hierarchy = registry.try_get<components::Hierarchy>(current);
                current = hierarchy ? hierarchy->parent : entt::null;
                depth++;
            }
            interactables.emplace_back(depth, entity);
        }

        // 排序：深度越大（越靠近前端）的排在前面
        std::sort(
            interactables.begin(), interactables.end(), [](const auto& a, const auto& b) { return a.first > b.first; });

        std::vector<entt::entity> result;
        for (const auto& pair : interactables)
        {
            result.push_back(pair.second);
        }
        return result;
    }

public:
    /**
     * @brief 处理输入事件和交互状态更新
     */
    void update() noexcept
    {
        auto& registry = ::utils::Registry::getInstance();
        const ImVec2 mousePos = input_utils::Input::getMousePosition();
        bool isMouseDown = input_utils::Input::isMouseButtonDown(0); // 假设左键

        // 1. 清除上一帧的 HoveredTag
        // 保证只有命中检测成功的实体才拥有 HoveredTag
        registry.clear<components::HoveredTag>();

        // 2. 获取按 Z-Order 排序的可交互实体列表
        auto interactables = getZOrderedInteractables(registry);
        entt::entity hitEntity = entt::null;

        // 3. 命中检测 (Z-Order from front to back)
        for (entt::entity entity : interactables)
        {
            // 实体必须可见
            if (!registry.any_of<components::VisibleTag>(entity)) continue;

            const auto& size = registry.get<const components::Size>(entity);
            const ImVec2 absPos = getAbsolutePosition(registry, entity);

            if (isPointInRect(mousePos, absPos, size))
            {
                hitEntity = entity;
                registry.emplace_or_replace<components::HoveredTag>(entity);

                // 阻止事件穿透：一旦命中，停止循环
                break;
            }
        }

        // 4. 处理 Active/Pressed 状态和 Click 事件
        if (hitEntity != entt::null)
        {
            if (isMouseDown)
            {
                // 鼠标按下：标记 Active (如果之前不是，则设置 m_activeEntity)
                registry.emplace_or_replace<components::ActiveTag>(hitEntity);
                if (m_activeEntity == entt::null)
                {
                    m_activeEntity = hitEntity;
                }
            }
            else // 鼠标抬起
            {
                // 检查是否发生了 Click (必须是 m_activeEntity 抬起且仍在 Hover)
                if (m_activeEntity == hitEntity)
                {
                    // 按钮点击事件：触发 ECS 事件
                    utils::Dispatcher::getInstance().trigger<events::ButtonClick>({hitEntity});

                    // 兼容：如果 Clickable 上挂了回调，直接调用
                    if (auto* clickable = registry.try_get<components::Clickable>(hitEntity))
                    {
                        if (clickable->enabled && clickable->onClick)
                        {
                            clickable->onClick(hitEntity);
                        }
                    }

                    // TODO: 对于 TextEditTag 等，可以在这里添加 FocusTag

                    // 触发后，将自身或父级标记为 LayoutDirtyTag，通知布局系统可能需要更新
                    registry.emplace_or_replace<components::LayoutDirtyTag>(hitEntity);
                }
            }
        }

        // 5. 状态清理
        if (!isMouseDown)
        {
            // 鼠标抬起时，清除所有 ActiveTag 并重置 m_activeEntity
            registry.clear<components::ActiveTag>();
            m_activeEntity = entt::null;
        }
        else if (m_activeEntity != entt::null && m_activeEntity != hitEntity)
        {
            // 鼠标拖出了最初按下的实体：移除其 ActiveTag
            registry.remove<components::ActiveTag>(m_activeEntity);
        }

        // TODO: 完善 Drag & Drop 逻辑 (需要 DraggableTag 和 DraggingTag)
    }
};

} // namespace ui::systems