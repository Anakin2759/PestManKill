/**
 * ************************************************************************
 *
 * @file UIHelper.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-11 (Updated)
 * @version 0.2
 * @brief UI辅助函数 (基于优化后的ECS组件)
 *
 * 提供便捷的UI操作函数，简化ECS UI的使用，并确保修改能触发布局更新。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include <algorithm>                            // For std::clamp
#include "src/client/components/UIComponents.h" // 包含所有数据组件
#include "src/client/components/UITags.h"       // 包含所有 Tag 组件
#include "src/client/components/UIDefine.h"     // 包含所有枚举

namespace ui::helper
{

// ===================== 状态修改后，触发布局更新 =====================
/**
 * @brief 标记此实体及其父实体为 LayoutDirty，通知布局系统重新计算。
 */
inline void markLayoutDirty(entt::registry& registry, entt::entity entity)
{
    entt::entity current = entity;
    while (current != entt::null && registry.valid(current))
    {
        registry.emplace_or_replace<components::LayoutDirtyTag>(current);
        const auto* hierarchy = registry.try_get<components::Hierarchy>(current);
        current = hierarchy ? hierarchy->parent : entt::null;
    }
}

// ===================== 尺寸与位置 =====================

/**
 * @brief 设置固定尺寸
 */
inline void setFixedSize(entt::registry& registry, entt::entity entity, float width, float height)
{
    if (!registry.valid(entity)) return;

    auto& size = registry.get_or_emplace<components::Size>(entity);
    size.width = width;
    size.height = height;
    size.autoSize = false;

    markLayoutDirty(registry, entity);
}

/**
 * @brief 设置位置
 * 注意：通常只用于根节点或不参与布局的自定义位置元素。
 */
inline void setPosition(entt::registry& registry, entt::entity entity, float x, float y)
{
    if (!registry.valid(entity)) return;

    auto& pos = registry.get_or_emplace<components::Position>(entity);
    pos.x = x;
    pos.y = y;
    // 我们优化后的 Position 组件中移除了 useCustomPosition 字段，
    // 布局系统通过检查 Position 组件是否存在来决定是否参与计算。
    // 这里设置 Position 组件即表示其位置被确定。

    markLayoutDirty(registry, entity);
}

// ===================== 可见性与样式 =====================

/**
 * @brief 设置可见性
 * 使用 VisibleTag 实现 ECS 哲学：可见则有 Tag，不可见则移除 Tag。
 */
inline void setVisible(entt::registry& registry, entt::entity entity, bool visible)
{
    if (!registry.valid(entity)) return;

    if (visible)
    {
        registry.emplace_or_replace<components::VisibleTag>(entity);
    }
    else
    {
        registry.remove<components::VisibleTag>(entity);
    }
}

/**
 * @brief 设置透明度
 */
inline void setAlpha(entt::registry& registry, entt::entity entity, float alpha)
{
    if (!registry.valid(entity)) return;

    auto& alphaComp = registry.get_or_emplace<components::Alpha>(entity);
    alphaComp.value = std::clamp(alpha, 0.0F, 1.0F);
}

/**
 * @brief 设置背景颜色
 */
inline void setBackgroundColor(entt::registry& registry, entt::entity entity, const ImVec4& color)
{
    if (!registry.valid(entity)) return;

    auto& bg = registry.get_or_emplace<components::Background>(entity);
    bg.color = color;
    bg.enabled = true;
}

// ===================== 布局配置 =====================

/**
 * @brief 设置布局间距
 */
inline void setLayoutSpacing(entt::registry& registry, entt::entity entity, float spacing)
{
    if (!registry.valid(entity)) return;

    auto* layout = registry.try_get<components::LayoutInfo>(entity);
    if (layout)
    {
        layout->spacing = std::max(0.0F, spacing);
        markLayoutDirty(registry, entity);
    }
}

/**
 * @brief 设置内边距 (Padding)
 * 原来的 setLayoutMargins 对应我们优化后的 Padding 组件。
 */
inline void setPadding(entt::registry& registry, entt::entity entity, float left, float top, float right, float bottom)
{
    if (!registry.valid(entity)) return;

    auto& padding = registry.get_or_emplace<components::Padding>(entity);
    // Padding 结构为 ImVec4(Top, Right, Bottom, Left)
    padding.values = ImVec4(top, right, bottom, left);

    markLayoutDirty(registry, entity);
}

/**
 * @brief 设置内边距 (统一值)
 */
inline void setPadding(entt::registry& registry, entt::entity entity, float padding)
{
    setPadding(registry, entity, padding, padding, padding, padding);
}

// ===================== 文本与交互 =====================

/**
 * @brief 设置按钮文本 (统一使用 Text 组件)
 */
inline void setButtonText(entt::registry& registry, entt::entity entity, const std::string& content)
{
    if (!registry.valid(entity)) return;

    // 按钮必须有 TextTag 或 ButtonTag
    if (registry.any_of<components::ButtonTag>(entity))
    {
        auto& text = registry.get_or_emplace<components::Text>(entity);
        text.content = content;
        markLayoutDirty(registry, entity);
    }
}

/**
 * @brief 设置按钮启用状态
 * 使用 DisabledTag 实现 ECS 哲学。
 */
inline void setButtonEnabled(entt::registry& registry, entt::entity entity, bool enabled)
{
    if (!registry.valid(entity)) return;

    if (enabled)
    {
        registry.remove<components::DisabledTag>(entity);
    }
    else
    {
        registry.emplace_or_replace<components::DisabledTag>(entity);
    }
}

/**
 * @brief 设置标签文本
 */
inline void setLabelText(entt::registry& registry, entt::entity entity, const std::string& content)
{
    if (!registry.valid(entity)) return;

    if (registry.any_of<components::LabelTag>(entity))
    {
        auto& text = registry.get_or_emplace<components::Text>(entity);
        text.content = content;
        markLayoutDirty(registry, entity);
    }
}

/**
 * @brief 设置文本颜色 (应用于 Text 和 TextEdit)
 */
inline void setTextColor(entt::registry& registry, entt::entity entity, const ImVec4& color)
{
    if (!registry.valid(entity)) return;

    if (auto* textComp = registry.try_get<components::Text>(entity))
    {
        textComp->color = color;
    }
    if (auto* textEdit = registry.try_get<components::TextEdit>(entity))
    {
        textEdit->textColor = color;
    }
}

/**
 * @brief 获取文本编辑框内容
 */
inline std::string getTextEditContent(entt::registry& registry, entt::entity entity)
{
    if (!registry.valid(entity)) return "";

    auto* textEdit = registry.try_get<components::TextEdit>(entity);
    return textEdit ? textEdit->buffer : ""; // 注意：我们优化后的 TextEdit 使用 buffer
}

/**
 * @brief 设置文本编辑框内容
 */
inline void setTextEditContent(entt::registry& registry, entt::entity entity, const std::string& content)
{
    if (!registry.valid(entity)) return;

    auto* textEdit = registry.try_get<components::TextEdit>(entity);
    if (textEdit)
    {
        textEdit->buffer = content; // 注意：我们优化后的 TextEdit 使用 buffer
    }
}

// ===================== 动画操作 =====================

/**
 * @brief 开始位置动画
 */
inline void startPositionAnimation(
    entt::registry& registry, entt::entity entity, const ImVec2& startPos, const ImVec2& endPos, float duration)
{
    if (!registry.valid(entity)) return;

    auto& time = registry.get_or_emplace<components::AnimationTime>(entity);
    time.duration = duration;
    time.elapsed = 0.0F;

    auto& posAnim = registry.get_or_emplace<components::AnimationPosition>(entity);
    posAnim.from = startPos;
    posAnim.to = endPos;

    registry.emplace_or_replace<components::AnimatingTag>(entity);
}

/**
 * @brief 开始透明度动画
 */
inline void
    startAlphaAnimation(entt::registry& registry, entt::entity entity, float startAlpha, float endAlpha, float duration)
{
    if (!registry.valid(entity)) return;

    auto& time = registry.get_or_emplace<components::AnimationTime>(entity);
    time.duration = duration;
    time.elapsed = 0.0F;

    auto& alphaAnim = registry.get_or_emplace<components::AnimationAlpha>(entity);
    alphaAnim.from = startAlpha;
    alphaAnim.to = endAlpha;

    registry.emplace_or_replace<components::AnimatingTag>(entity);
}

/**
 * @brief 停止动画
 */
inline void stopAnimation(entt::registry& registry, entt::entity entity)
{
    if (!registry.valid(entity)) return;

    registry.remove<components::AnimatingTag>(entity);
    // 也可以选择移除 AnimationTime/AnimationAlpha 等，但保留这些组件以便立即开始新动画更方便。
}

// ===================== 层级操作 (核心功能补全) =====================

/**
 * @brief 添加子实体到父容器中 (核心 Hierarchy 操作)
 */
inline void addChild(entt::registry& registry, entt::entity parent, entt::entity child)
{
    if (!registry.valid(parent) || !registry.valid(child)) return;

    // 1. 设置子实体的 Hierarchy
    auto& childHierarchy = registry.get_or_emplace<components::Hierarchy>(child);

    // 如果子实体已经有父级，则先从旧父级移除
    if (childHierarchy.parent != entt::null && childHierarchy.parent != parent)
    {
        // 实际应用中需要一个 removeChild 函数
        // removeChild(registry, childHierarchy.parent, child);
    }
    childHierarchy.parent = parent;

    // 2. 更新父实体的 Hierarchy
    auto& parentHierarchy = registry.get_or_emplace<components::Hierarchy>(parent);

    // 确保子实体不在列表中
    auto& children = parentHierarchy.children;
    if (std::find(children.begin(), children.end(), child) == children.end())
    {
        children.push_back(child);
    }

    // 3. 标记父级为脏，触发布局计算
    markLayoutDirty(registry, parent);
}

/**
 * @brief 移除子实体 (从 Hierarchy 中断开，但实体本身可能仍然存在)
 */
inline void removeChild(entt::registry& registry, entt::entity parent, entt::entity child)
{
    if (!registry.valid(parent) || !registry.valid(child)) return;

    auto* parentHierarchy = registry.try_get<components::Hierarchy>(parent);
    auto* childHierarchy = registry.try_get<components::Hierarchy>(child);

    if (parentHierarchy && childHierarchy && childHierarchy->parent == parent)
    {
        // 1. 从父级的 children 列表中移除
        auto& children = parentHierarchy->children;
        children.erase(std::remove(children.begin(), children.end(), child), children.end());

        // 2. 清除子级的 parent 指针
        childHierarchy->parent = entt::null;

        // 3. 标记父级为脏
        markLayoutDirty(registry, parent);
    }
}

// ===================== 实用工具函数 =====================

/**
 * @brief 检查是否对齐方式包含指定标志
 */
inline bool hasAlignment(components::Alignment value, components::Alignment flag)
{
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

} // namespace ui::helper