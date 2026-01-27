/**
 * ************************************************************************
 *
 * @file CompenentsTraits.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-23
 * @version 0.1
 * @brief 利用模板元编程实现组件特性检测
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include "../common/Components.hpp"
#include "../common/Tags.hpp"
#include "Contains.hpp"

namespace ui::traits
{

// ===================== 组件列表 =====================
using DataComponents = TypeList<components::BaseInfo,
                                components::Size,
                                components::Position,
                                components::CanvasSize,
                                components::Margin,
                                components::Padding,
                                components::Background,
                                components::Border,
                                components::Shadow,
                                components::Alpha,
                                components::Hierarchy,
                                components::ScrollArea,
                                components::LayoutInfo,
                                components::Spacer,
                                components::Text,
                                components::TextEdit,
                                components::Image,
                                components::Clickable,
                                components::Hoverable,
                                components::Pressable,
                                components::Checkable,
                                components::ButtonState,
                                components::AnimationTime,
                                components::AnimationPosition,
                                components::AnimationAlpha,
                                components::Window,
                                components::Arrow,
                                components::ListArea,
                                components::TableInfo,
                                components::LineInfo,
                                components::Title,
                                components::Targetable,
                                components::SliderInfo,
                                components::ScrollBar,
                                components::ProgressBar>;

// ===================== 标签列表 =====================
using TagComponents = TypeList<components::MainWidgetTag,
                               components::UiTag,
                               components::ButtonTag,
                               components::LabelTag,
                               components::TextTag,
                               components::TextEditTag,
                               components::ImageTag,
                               components::WindowTag,
                               components::DialogTag,
                               components::SpacerTag,
                               components::ArrowTag,
                               components::LineTag,
                               components::ListAreaTag,
                               components::TableTag,
                               components::ClickableTag,
                               components::DraggableTag,
                               components::HoveredTag,
                               components::ActiveTag,
                               components::DisabledTag,
                               components::FocusedTag,
                               components::VisibleTag,
                               components::LayoutDirtyTag,
                               components::RenderDirtyTag,
                               components::AnimatingTag>;

// ===================== 元编程辅助工具 =====================

/**
 * @brief 辅助变量模板 (更直接的定义方式)
 */
template <typename T>
inline constexpr bool is_ui_component_v = contains_v<T, DataComponents> || contains_v<T, TagComponents>; // NOLINT

template <typename T>
inline constexpr bool is_ui_tag_v = contains_v<T, TagComponents>; // NOLINT

/**
 * @brief 概念 Definitions (C++20)
 */
template <typename T>
concept Component = is_ui_component_v<T>;

template <typename T>
concept UiTag = is_ui_tag_v<T>;

template <typename T>
concept ComponentOrUiTag = Component<T> || UiTag<T>;

} // namespace ui::traits
