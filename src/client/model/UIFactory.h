/**
 * ************************************************************************
 *
 * @file UIFactory.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief UI工厂函数 纯函数直接调用就行
 *
 * 提供创建各种UI元素的便捷方法
 * 使用ECS模式创建实体和组件
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include <string>
#include <functional>
#include "src/client/components/UIComponents.h"
#include "src/client/components/InputComponents.h"
#include "src/client/events/UIEvents.h"
#include "src/client/utils/utils.h"

namespace ui::factory
{

/**
 * @brief 创建按钮
 */
entt::entity CreateButton(const std::string& text)
{
    auto& registry = utils::Registry::getInstance();
    auto entity = registry.create();

    registry.emplace<components::Position>(entity);
    registry.emplace<components::Size>(entity);
    registry.emplace<components::Visibility>(entity);
    registry.emplace<components::RenderGuard>(entity);
    registry.emplace<components::Clickable>(entity);
    registry.get<components::ShowText>(entity);

    utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
    return entity;
}

/**
 * @brief 创建文本标签
 */
entt::entity CreateLabel(const std::string& text)
{
    auto entity = utils::Registry::getInstance().create();

    utils::Registry::getInstance().emplace<components::Position>(entity);
    utils::Registry::getInstance().emplace<components::Size>(entity);
    utils::Registry::getInstance().emplace<components::Visibility>(entity);
    utils::Registry::getInstance().emplace<components::RenderGuard>(entity);
    utils::Registry::getInstance().emplace<components::LabelTag>(entity);

    utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
    return entity;
}

/**
 * @brief 创建文本编辑框
 */
entt::entity CreateTextEdit(const std::string& placeholder = "", bool multiline = false)
{
    auto entity = utils::Registry::getInstance().create();

    utils::Registry::getInstance().emplace<components::Position>(entity);
    utils::Registry::getInstance().emplace<components::Size>(entity);
    utils::Registry::getInstance().emplace<components::Visibility>(entity);
    utils::Registry::getInstance().emplace<components::RenderGuard>(entity);
    utils::Registry::getInstance().emplace<components::TextEditTag>(entity);

    auto& textEdit = utils::Registry::getInstance().emplace<components::TextEdit>(entity);
    textEdit.placeholder = placeholder;
    textEdit.multiline = multiline;

    utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
    return entity;
}

/**
 * @brief 创建图像
 */
entt::entity CreateImage(void* textureId)
{
    auto entity = utils::Registry::getInstance().create();

    utils::Registry::getInstance().emplace<components::Position>(entity);
    utils::Registry::getInstance().emplace<components::Size>(entity);
    utils::Registry::getInstance().emplace<components::Visibility>(entity);
    utils::Registry::getInstance().emplace<components::RenderGuard>(entity);
    utils::Registry::getInstance().emplace<components::ImageTag>(entity);

    auto& image = utils::Registry::getInstance().emplace<components::Image>(entity);
    image.textureId = textureId;

    utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
    return entity;
}

/**
 * @brief 创建箭头
 */
entt::entity CreateArrow(const ImVec2& start, const ImVec2& end)
{
    auto entity = utils::Registry::getInstance().create();

    utils::Registry::getInstance().emplace<components::Position>(entity);
    utils::Registry::getInstance().emplace<components::Size>(entity);
    utils::Registry::getInstance().emplace<components::Visibility>(entity);
    utils::Registry::getInstance().emplace<components::RenderGuard>(entity);
    utils::Registry::getInstance().emplace<components::Widget>(entity);

    auto& arrow = utils::Registry::getInstance().emplace<components::ArrowTag>(entity);
    arrow.startPoint = start;
    arrow.endPoint = end;

    utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
    return entity;
}

/**
 * @brief 创建水平布局
 */
entt::entity CreateHBoxLayout()
{
    auto entity = utils::Registry::getInstance().create();

    utils::Registry::getInstance().emplace<components::Position>(entity);
    utils::Registry::getInstance().emplace<components::Size>(entity);
    utils::Registry::getInstance().emplace<components::Visibility>(entity);
    utils::Registry::getInstance().emplace<components::RenderGuard>(entity);
    utils::Registry::getInstance().emplace<components::Widget>(entity);

    auto& layout = utils::Registry::getInstance().emplace<components::Layout>(entity);
    layout.direction = components::LayoutDirection::HORIZONTAL;

    utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
    return entity;
}

/**
 * @brief 创建垂直布局
 */
entt::entity CreateVBoxLayout()
{
    auto entity = utils::Registry::getInstance().create();

    utils::Registry::getInstance().emplace<components::Position>(entity);
    utils::Registry::getInstance().emplace<components::Size>(entity);
    utils::Registry::getInstance().emplace<components::Visibility>(entity);
    utils::Registry::getInstance().emplace<components::RenderGuard>(entity);
    utils::Registry::getInstance().emplace<components::Widget>(entity);

    auto& layout = utils::Registry::getInstance().emplace<components::Layout>(entity);
    layout.direction = components::LayoutDirection::VERTICAL;

    utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
    return entity;
}

/**
 * @brief 创建间隔器
 */
entt::entity CreateSpacer(int stretchFactor = 1, bool horizontal = true)
{
    auto entity = utils::Registry::getInstance().create();

    utils::Registry::getInstance().emplace<components::Position>(entity);
    utils::Registry::getInstance().emplace<components::Size>(entity);
    utils::Registry::getInstance().emplace<components::Visibility>(entity);
    utils::Registry::getInstance().emplace<components::Widget>(entity);

    auto& spacer = utils::Registry::getInstance().emplace<components::Spacer>(entity);
    spacer.stretchFactor = stretchFactor;
    spacer.horizontal = horizontal;

    utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
    return entity;
}

/**
 * @brief 创建对话框
 */
#ifdef CreateDialog
#undef CreateDialog
#endif
entt::entity CreateDialog(std::string_view title)
{
    auto entity = utils::Registry::getInstance().create();

    utils::Registry::getInstance().emplace<components::Position>(entity);
    utils::Registry::getInstance().emplace<components::Size>(entity);
    utils::Registry::getInstance().emplace<components::Visibility>(entity);
    utils::Registry::getInstance().emplace<components::RenderGuard>(entity);
    utils::Registry::getInstance().emplace<components::Widget>(entity);

    auto& dialog = utils::Registry::getInstance().emplace<components::Dialog>(entity);
    dialog.title = title;

    utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
    return entity;
}

} // namespace ui::factory
