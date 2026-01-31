/**
 * ************************************************************************
 *
 * @file GlobalContext.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-31
 * @version 0.1
 * @brief 全局渲染上下文组件定义
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include "Types.hpp"
#include <entt/entt.hpp>

namespace ui::globalContext
{
/**
 * @brief 全局获取帧间隔状态组件
 */
struct FrameContext
{
    using is_component_tag = void;
    uint32_t intervalMs = 0; // 时间间隔（毫秒）
    uint8_t frameSlot = 0;
};

/**
 * @brief 全局 UI 状态（从 StateSystem 提取）
 */
struct StateContext
{
    using is_component_tag = void;
    Vec2 latestMousePosition{0.0F, 0.0F};   // 全局最新鼠标位置
    Vec2 latestMouseDelta{0.0F, 0.0F};      // 全局最新鼠标移动增量
    Vec2 latestScrollDelta{0.0F, 0.0F};     // 全局最新滚轮滚动增量
    entt::entity focusedEntity{entt::null}; // 当前获得焦点的实体
    entt::entity activeEntity{entt::null};  // 当前处于活动状态的实体（鼠标按下）
    entt::entity hoveredEntity{entt::null}; // 当前悬停的实体

    // 拖拽状态
    bool isDraggingScrollbar{false};
    entt::entity dragScrollEntity{entt::null};
    Vec2 dragStartMousePos{0.0f, 0.0f};
    Vec2 dragStartScrollOffset{0.0f, 0.0f};
    bool isVerticalDrag{true};
    float dragTrackLength{0.0f}; // 轨道可活动区域长度
    float dragThumbSize{0.0f};   // 滑块大小

    /**
     * @brief 重置所有状态
     */
    void reset()
    {
        latestMousePosition = Vec2{0.0F, 0.0F};
        latestMouseDelta = Vec2{0.0F, 0.0F};
        latestScrollDelta = Vec2{0.0F, 0.0F};
        focusedEntity = entt::null;
        activeEntity = entt::null;
        hoveredEntity = entt::null;

        isDraggingScrollbar = false;
        dragScrollEntity = entt::null;
    }
};

} // namespace ui::globalContext