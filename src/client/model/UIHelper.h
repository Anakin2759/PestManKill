/**
 * ************************************************************************
 *
 * @file UIHelper.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief UI辅助函数
 *
 * 提供便捷的UI操作函数，简化ECS UI的使用
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include "src/client/components/UIComponents.h"

namespace ui::helper
{

/**
 * @brief 设置固定尺寸
 */
inline void setFixedSize(entt::registry& registry, entt::entity entity, float width, float height)
{
    if (!registry.valid(entity))
    {
        return;
    }

    auto& size = registry.get_or_emplace<components::Size>(entity);
    size.width = width;
    size.height = height;
    size.useFixedSize = true;
}

/**
 * @brief 设置位置
 */
inline void setPosition(entt::registry& registry, entt::entity entity, float x, float y)
{
    if (!registry.valid(entity))
    {
        return;
    }

    auto& pos = registry.get_or_emplace<components::Position>(entity);
    pos.x = x;
    pos.y = y;
    pos.useCustomPosition = true;
}

/**
 * @brief 设置可见性
 */
inline void setVisible(entt::registry& registry, entt::entity entity, bool visible)
{
    if (!registry.valid(entity))
    {
        return;
    }

    auto& visibility = registry.get_or_emplace<components::Visibility>(entity);
    visibility.visible = visible;
}

/**
 * @brief 设置透明度
 */
inline void setAlpha(entt::registry& registry, entt::entity entity, float alpha)
{
    if (!registry.valid(entity))
    {
        return;
    }

    auto& visibility = registry.get_or_emplace<components::Visibility>(entity);
    visibility.alpha = std::clamp(alpha, 0.0F, 1.0F);
}

/**
 * @brief 设置背景颜色
 */
inline void setBackgroundColor(entt::registry& registry, entt::entity entity, const ImVec4& color)
{
    if (!registry.valid(entity))
    {
        return;
    }

    auto& bg = registry.get_or_emplace<components::Background>(entity);
    bg.color = color;
    bg.enabled = true;
}

/**
 * @brief 设置布局间距
 */
inline void setLayoutSpacing(entt::registry& registry, entt::entity entity, float spacing)
{
    if (!registry.valid(entity))
    {
        return;
    }

    auto* layout = registry.try_get<components::Layout>(entity);
    if (layout)
    {
        layout->spacing = std::max(0.0F, spacing);
    }
}

/**
 * @brief 设置布局边距
 */
inline void
    setLayoutMargins(entt::registry& registry, entt::entity entity, float left, float top, float right, float bottom)
{
    if (!registry.valid(entity))
    {
        return;
    }

    auto* layout = registry.try_get<components::Layout>(entity);
    if (layout)
    {
        layout->margins = ImVec4(left, top, right, bottom);
    }
}

/**
 * @brief 设置布局边距（统一值）
 */
inline void setLayoutMargins(entt::registry& registry, entt::entity entity, float margins)
{
    setLayoutMargins(registry, entity, margins, margins, margins, margins);
}

/**
 * @brief 设置按钮文本
 */
inline void setButtonText(entt::registry& registry, entt::entity entity, const std::string& text)
{
    if (!registry.valid(entity))
    {
        return;
    }

    auto* button = registry.try_get<components::Button>(entity);
    if (button)
    {
        button->text = text;
    }
}

/**
 * @brief 设置按钮启用状态
 */
inline void setButtonEnabled(entt::registry& registry, entt::entity entity, bool enabled)
{
    if (!registry.valid(entity))
    {
        return;
    }

    auto* button = registry.try_get<components::Button>(entity);
    if (button)
    {
        button->enabled = enabled;
    }
}

/**
 * @brief 设置标签文本
 */
inline void setLabelText(entt::registry& registry, entt::entity entity, const std::string& text)
{
    if (!registry.valid(entity))
    {
        return;
    }

    auto* label = registry.try_get<components::Label>(entity);
    if (label)
    {
        label->text = text;
    }
}

/**
 * @brief 设置文本颜色
 */
inline void setTextColor(entt::registry& registry, entt::entity entity, const ImVec4& color)
{
    if (!registry.valid(entity))
    {
        return;
    }

    if (auto* label = registry.try_get<components::Label>(entity))
    {
        label->textColor = color;
    }
    else if (auto* textEdit = registry.try_get<components::TextEdit>(entity))
    {
        textEdit->textColor = color;
    }
}

/**
 * @brief 获取文本编辑框内容
 */
inline std::string getTextEditContent(entt::registry& registry, entt::entity entity)
{
    if (!registry.valid(entity))
    {
        return "";
    }

    auto* textEdit = registry.try_get<components::TextEdit>(entity);
    return textEdit ? textEdit->text : "";
}

/**
 * @brief 设置文本编辑框内容
 */
inline void setTextEditContent(entt::registry& registry, entt::entity entity, const std::string& text)
{
    if (!registry.valid(entity))
    {
        return;
    }

    auto* textEdit = registry.try_get<components::TextEdit>(entity);
    if (textEdit)
    {
        textEdit->text = text;
    }
}

/**
 * @brief 开始位置动画
 */
inline void startPositionAnimation(
    entt::registry& registry, entt::entity entity, const ImVec2& startPos, const ImVec2& endPos, float duration)
{
    if (!registry.valid(entity))
    {
        return;
    }

    auto& animation = registry.get_or_emplace<components::Animation>(entity);
    animation.active = true;
    animation.duration = duration;
    animation.elapsed = 0.0F;

    auto& posAnim = registry.get_or_emplace<components::PositionAnimation>(entity);
    posAnim.startPos = startPos;
    posAnim.endPos = endPos;
    posAnim.progress = 0.0F;
}

/**
 * @brief 开始透明度动画
 */
inline void
    startAlphaAnimation(entt::registry& registry, entt::entity entity, float startAlpha, float endAlpha, float duration)
{
    if (!registry.valid(entity))
    {
        return;
    }

    auto& animation = registry.get_or_emplace<components::Animation>(entity);
    animation.active = true;
    animation.duration = duration;
    animation.elapsed = 0.0F;

    auto& alphaAnim = registry.get_or_emplace<components::AlphaAnimation>(entity);
    alphaAnim.startAlpha = startAlpha;
    alphaAnim.endAlpha = endAlpha;
    alphaAnim.progress = 0.0F;
}

/**
 * @brief 停止动画
 */
inline void stopAnimation(entt::registry& registry, entt::entity entity)
{
    if (!registry.valid(entity))
    {
        return;
    }

    auto* animation = registry.try_get<components::Animation>(entity);
    if (animation)
    {
        animation->active = false;
    }
}

/**
 * @brief 检查是否对齐方式包含指定标志
 */
inline bool hasAlignment(components::Alignment value, components::Alignment flag)
{
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

} // namespace ui::helper
