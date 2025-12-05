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
    explicit UIFactory(entt::registry& registry) : m_registry(registry) {}

    /**
     * @brief 创建按钮
     */
    entt::entity createButton(const std::string& text, std::function<void()> onClick = nullptr)
    {
        auto entity = m_registry.create();

        m_registry.emplace<components::Position>(entity);
        m_registry.emplace<components::Size>(entity);
        m_registry.emplace<components::Visibility>(entity);
        m_registry.emplace<components::RenderState>(entity);
        m_registry.emplace<components::ButtonTag>(entity);

        auto& button = m_registry.emplace<components::Button>(entity);
        button.text = text;
        button.onClick = std::move(onClick);
        button.uniqueId = "##Button_" + std::to_string(m_buttonIdCounter++);

        utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
        return entity;
    }

    /**
     * @brief 创建文本标签
     */
    entt::entity createLabel(const std::string& text)
    {
        auto entity = m_registry.create();

        m_registry.emplace<components::Position>(entity);
        m_registry.emplace<components::Size>(entity);
        m_registry.emplace<components::Visibility>(entity);
        m_registry.emplace<components::RenderState>(entity);
        m_registry.emplace<components::LabelTag>(entity);

        auto& label = m_registry.emplace<components::Label>(entity);
        label.text = text;

        utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
        return entity;
    }

    /**
     * @brief 创建文本编辑框
     */
    entt::entity createTextEdit(const std::string& placeholder = "", bool multiline = false)
    {
        auto entity = m_registry.create();

        m_registry.emplace<components::Position>(entity);
        m_registry.emplace<components::Size>(entity);
        m_registry.emplace<components::Visibility>(entity);
        m_registry.emplace<components::RenderState>(entity);
        m_registry.emplace<components::TextEditTag>(entity);

        auto& textEdit = m_registry.emplace<components::TextEdit>(entity);
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
        auto entity = m_registry.create();

        m_registry.emplace<components::Position>(entity);
        m_registry.emplace<components::Size>(entity);
        m_registry.emplace<components::Visibility>(entity);
        m_registry.emplace<components::RenderState>(entity);
        m_registry.emplace<components::ImageTag>(entity);

        auto& image = m_registry.emplace<components::Image>(entity);
        image.textureId = textureId;

        utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
        return entity;
    }

    /**
     * @brief 创建箭头
     */
    entt::entity createArrow(const ImVec2& start, const ImVec2& end)
    {
        auto entity = m_registry.create();

        m_registry.emplace<components::Position>(entity);
        m_registry.emplace<components::Size>(entity);
        m_registry.emplace<components::Visibility>(entity);
        m_registry.emplace<components::RenderState>(entity);
        m_registry.emplace<components::ArrowTag>(entity);

        auto& arrow = m_registry.emplace<components::Arrow>(entity);
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
        auto entity = m_registry.create();

        m_registry.emplace<components::Position>(entity);
        m_registry.emplace<components::Size>(entity);
        m_registry.emplace<components::Visibility>(entity);
        m_registry.emplace<components::RenderState>(entity);
        m_registry.emplace<components::LayoutTag>(entity);

        auto& layout = m_registry.emplace<components::Layout>(entity);
        layout.direction = components::LayoutDirection::HORIZONTAL;

        utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
        return entity;
    }

    /**
     * @brief 创建垂直布局
     */
    entt::entity createVBoxLayout()
    {
        auto entity = m_registry.create();

        m_registry.emplace<components::Position>(entity);
        m_registry.emplace<components::Size>(entity);
        m_registry.emplace<components::Visibility>(entity);
        m_registry.emplace<components::RenderState>(entity);
        m_registry.emplace<components::LayoutTag>(entity);

        auto& layout = m_registry.emplace<components::Layout>(entity);
        layout.direction = components::LayoutDirection::VERTICAL;

        utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
        return entity;
    }

    /**
     * @brief 创建间隔器
     */
    entt::entity createSpacer(int stretchFactor = 1, bool horizontal = true)
    {
        auto entity = m_registry.create();

        m_registry.emplace<components::Position>(entity);
        m_registry.emplace<components::Size>(entity);
        m_registry.emplace<components::Visibility>(entity);
        m_registry.emplace<components::SpacerTag>(entity);

        auto& spacer = m_registry.emplace<components::Spacer>(entity);
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
        auto entity = m_registry.create();

        m_registry.emplace<components::Position>(entity);
        m_registry.emplace<components::Size>(entity);
        m_registry.emplace<components::Visibility>(entity);
        m_registry.emplace<components::RenderState>(entity);
        m_registry.emplace<components::DialogTag>(entity);

        auto& dialog = m_registry.emplace<components::Dialog>(entity);
        dialog.title = title;

        utils::Dispatcher::getInstance().enqueue<events::WidgetCreated>(entity);
        return entity;
    }

    /**
     * @brief 添加子元素
     */
    void addChild(entt::entity parent, entt::entity child)
    {
        if (!m_registry.valid(parent) || !m_registry.valid(child))
        {
            return;
        }

        // 防止循环引用
        if (parent == child)
        {
            return;
        }

        auto& parentHierarchy = m_registry.get_or_emplace<components::Hierarchy>(parent);
        auto& childHierarchy = m_registry.get_or_emplace<components::Hierarchy>(child);

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
        if (!m_registry.valid(parent) || !m_registry.valid(child))
        {
            return;
        }

        auto* parentHierarchy = m_registry.try_get<components::Hierarchy>(parent);
        if (!parentHierarchy)
        {
            return;
        }

        auto it = std::find(parentHierarchy->children.begin(), parentHierarchy->children.end(), child);
        if (it != parentHierarchy->children.end())
        {
            parentHierarchy->children.erase(it);
        }

        auto* childHierarchy = m_registry.try_get<components::Hierarchy>(child);
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
        if (!m_registry.valid(layout) || !m_registry.valid(widget))
        {
            return;
        }

        auto* layoutComp = m_registry.try_get<components::Layout>(layout);
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
        if (!m_registry.valid(layout))
        {
            return;
        }

        auto* layoutComp = m_registry.try_get<components::Layout>(layout);
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
        if (!m_registry.valid(entity))
        {
            return;
        }

        // 递归销毁子元素
        if (auto* hierarchy = m_registry.try_get<components::Hierarchy>(entity))
        {
            auto children = hierarchy->children; // 复制一份避免迭代器失效
            for (auto child : children)
            {
                destroyWidget(child);
            }
        }

        utils::Dispatcher::getInstance().enqueue<events::WidgetDestroyed>(entity);
        m_registry.destroy(entity);
    }

private:
    entt::registry& m_registry;

    size_t m_buttonIdCounter = 0;
    size_t m_textEditIdCounter = 0;
};

} // namespace ui
