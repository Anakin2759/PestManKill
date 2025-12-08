/**
 * ************************************************************************
 *
 * @file UIFactory.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief UI工厂类
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
#include "src/client/events/UIEvents.h"
#include "src/client/utils/Dispatcher.h"

namespace ui
{

class UIFactory
{
public:
    /**
     * @brief 创建按钮
     */
    entt::entity createButton(const std::string& text)
    {
        auto& registry = utils::Registry::getInstance();
        auto entity = registry.create();

        registry.emplace<components::Position>(entity);
        registry.emplace<components::Size>(entity);
        registry.emplace<components::Visibility>(entity);
        registry.emplace<components::RenderState>(entity);
        registry.emplace<components::ButtonTag>(entity);
        registry.emplace<components::Clickable>(entity);
        registry.get<components::ShowText>(entity);

        utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
        return entity;
    }

    /**
     * @brief 创建文本标签
     */
    entt::entity createLabel(const std::string& text)
    {
        auto entity = utils::Registry::getInstance().create();

        utils::Registry::getInstance().emplace<components::Position>(entity);
        utils::Registry::getInstance().emplace<components::Size>(entity);
        utils::Registry::getInstance().emplace<components::Visibility>(entity);
        utils::Registry::getInstance().emplace<components::RenderState>(entity);
        utils::Registry::getInstance().emplace<components::LabelTag>(entity);

        utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
        return entity;
    }

    /**
     * @brief 创建文本编辑框
     */
    entt::entity createTextEdit(const std::string& placeholder = "", bool multiline = false)
    {
        auto entity = utils::Registry::getInstance().create();

        utils::Registry::getInstance().emplace<components::Position>(entity);
        utils::Registry::getInstance().emplace<components::Size>(entity);
        utils::Registry::getInstance().emplace<components::Visibility>(entity);
        utils::Registry::getInstance().emplace<components::RenderState>(entity);
        utils::Registry::getInstance().emplace<components::TextEditTag>(entity);

        auto& textEdit = utils::Registry::getInstance().emplace<components::TextEdit>(entity);
        textEdit.placeholder = placeholder;
        textEdit.multiline = multiline;
        textEdit.uniqueId = "##TextEdit_" + std::to_string(m_textEditIdCounter++);

        utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
        return entity;
    }

    /**
     * @brief 创建图像
     */
    entt::entity createImage(void* textureId)
    {
        auto entity = utils::Registry::getInstance().create();

        utils::Registry::getInstance().emplace<components::Position>(entity);
        utils::Registry::getInstance().emplace<components::Size>(entity);
        utils::Registry::getInstance().emplace<components::Visibility>(entity);
        utils::Registry::getInstance().emplace<components::RenderState>(entity);
        utils::Registry::getInstance().emplace<components::ImageTag>(entity);

        auto& image = utils::Registry::getInstance().emplace<components::Image>(entity);
        image.textureId = textureId;

        utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
        return entity;
    }

    /**
     * @brief 创建箭头
     */
    entt::entity createArrow(const ImVec2& start, const ImVec2& end)
    {
        auto entity = utils::Registry::getInstance().create();

        utils::Registry::getInstance().emplace<components::Position>(entity);
        utils::Registry::getInstance().emplace<components::Size>(entity);
        utils::Registry::getInstance().emplace<components::Visibility>(entity);
        utils::Registry::getInstance().emplace<components::RenderState>(entity);
        utils::Registry::getInstance().emplace<components::ArrowTag>(entity);

        auto& arrow = utils::Registry::getInstance().emplace<components::Arrow>(entity);
        arrow.startPoint = start;
        arrow.endPoint = end;

        utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
        return entity;
    }

    /**
     * @brief 创建水平布局
     */
    entt::entity createHBoxLayout()
    {
        auto entity = utils::Registry::getInstance().create();

        utils::Registry::getInstance().emplace<components::Position>(entity);
        utils::Registry::getInstance().emplace<components::Size>(entity);
        utils::Registry::getInstance().emplace<components::Visibility>(entity);
        utils::Registry::getInstance().emplace<components::RenderState>(entity);
        utils::Registry::getInstance().emplace<components::LayoutTag>(entity);

        auto& layout = utils::Registry::getInstance().emplace<components::Layout>(entity);
        layout.direction = components::LayoutDirection::HORIZONTAL;

        utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
        return entity;
    }

    /**
     * @brief 创建垂直布局
     */
    entt::entity createVBoxLayout()
    {
        auto entity = utils::Registry::getInstance().create();

        utils::Registry::getInstance().emplace<components::Position>(entity);
        utils::Registry::getInstance().emplace<components::Size>(entity);
        utils::Registry::getInstance().emplace<components::Visibility>(entity);
        utils::Registry::getInstance().emplace<components::RenderState>(entity);
        utils::Registry::getInstance().emplace<components::LayoutTag>(entity);

        auto& layout = utils::Registry::getInstance().emplace<components::Layout>(entity);
        layout.direction = components::LayoutDirection::VERTICAL;

        utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
        return entity;
    }

    /**
     * @brief 创建间隔器
     */
    entt::entity createSpacer(int stretchFactor = 1, bool horizontal = true)
    {
        auto entity = utils::Registry::getInstance().create();

        utils::Registry::getInstance().emplace<components::Position>(entity);
        utils::Registry::getInstance().emplace<components::Size>(entity);
        utils::Registry::getInstance().emplace<components::Visibility>(entity);
        utils::Registry::getInstance().emplace<components::SpacerTag>(entity);

        auto& spacer = utils::Registry::getInstance().emplace<components::Spacer>(entity);
        spacer.stretchFactor = stretchFactor;
        spacer.isHorizontal = horizontal;

        utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
        return entity;
    }

    /**
     * @brief 创建对话框
     */
    entt::entity createDialog(const std::string& title)
    {
        auto entity = utils::Registry::getInstance().create();

        utils::Registry::getInstance().emplace<components::Position>(entity);
        utils::Registry::getInstance().emplace<components::Size>(entity);
        utils::Registry::getInstance().emplace<components::Visibility>(entity);
        utils::Registry::getInstance().emplace<components::RenderState>(entity);
        utils::Registry::getInstance().emplace<components::DialogTag>(entity);

        auto& dialog = utils::Registry::getInstance().emplace<components::Dialog>(entity);
        dialog.title = title;

        utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
        return entity;
    }

    /**
     * @brief 添加子元素
     */
    void addChild(entt::entity parent, entt::entity child)
    {
        if (!utils::Registry::getInstance().valid(parent) || !utils::Registry::getInstance().valid(child))
        {
            return;
        }

        // 防止循环引用
        if (parent == child)
        {
            return;
        }

        auto& parentHierarchy = utils::Registry::getInstance().get_or_emplace<components::Hierarchy>(parent);
        auto& childHierarchy = utils::Registry::getInstance().get_or_emplace<components::Hierarchy>(child);

        // 如果子元素已经有父元素，先移除
        if (childHierarchy.parent != entt::null)
        {
            removeChild(childHierarchy.parent, child);
        }

        parentHierarchy.children.push_back(child);
        childHierarchy.parent = parent;
    }

    /**
     * @brief 移除子元素
     */
    void removeChild(entt::entity parent, entt::entity child)
    {
        if (!utils::Registry::getInstance().valid(parent) || !utils::Registry::getInstance().valid(child))
        {
            return;
        }

        auto* parentHierarchy = utils::Registry::getInstance().try_get<components::Hierarchy>(parent);
        if (!parentHierarchy)
        {
            return;
        }

        auto it = std::find(parentHierarchy->children.begin(), parentHierarchy->children.end(), child);
        if (it != parentHierarchy->children.end())
        {
            parentHierarchy->children.erase(it);
        }

        auto* childHierarchy = utils::Registry::getInstance().try_get<components::Hierarchy>(child);
        if (childHierarchy)
        {
            childHierarchy->parent = entt::null;
        }
    }

    /**
     * @brief 添加元素到布局
     */
    void addWidgetToLayout(entt::entity layout,
                           entt::entity widget,
                           int stretch = 0,
                           components::Alignment alignment = components::Alignment::LEFT)
    {
        if (!utils::Registry::getInstance().valid(layout) || !utils::Registry::getInstance().valid(widget))
        {
            return;
        }

        auto* layoutComp = utils::Registry::getInstance().try_get<components::Layout>(layout);
        if (!layoutComp)
        {
            return;
        }

        layoutComp->items.push_back({widget, stretch, alignment});
        addChild(layout, widget);
    }

    /**
     * @brief 添加弹性空间到布局
     */
    void addStretchToLayout(entt::entity layout, int stretch = 1)
    {
        if (!utils::Registry::getInstance().valid(layout))
        {
            return;
        }

        auto* layoutComp = utils::Registry::getInstance().try_get<components::Layout>(layout);
        if (!layoutComp)
        {
            return;
        }

        layoutComp->items.push_back({entt::null, stretch, components::Alignment::LEFT});
    }

    /**
     * @brief 销毁UI元素（递归销毁子元素）
     */
    void destroyWidget(entt::entity entity)
    {
        if (!utils::Registry::getInstance().valid(entity))
        {
            return;
        }

        // 递归销毁子元素
        if (auto* hierarchy = utils::Registry::getInstance().try_get<components::Hierarchy>(entity))
        {
            auto children = hierarchy->children; // 复制一份避免迭代器失效
            for (auto child : children)
            {
                destroyWidget(child);
            }
        }

        utils::Dispatcher::getInstance().enqueue<events::WidgetDestroyed>(entity);
        utils::Registry::getInstance().destroy(entity);
    }

private:
    size_t m_buttonIdCounter = 0;
    size_t m_textEditIdCounter = 0;
};

} // namespace ui
