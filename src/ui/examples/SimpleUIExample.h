/**
 * ************************************************************************
 *
 * @file SimpleUIExample.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-23
 * @version 0.1
 * @brief UI 模块使用示例
 *
 * 展示如何使用优化后的 UI 模块创建简单的界面
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "src/ui/core/Application.h"
#include "src/ui/core/Helper.h"
#include "src/ui/core/Factory.h"
#include "src/ui/components/Components.h"
#include "src/ui/components/Tags.h"

namespace ui::examples
{

/**
 * @brief 简单 UI 示例应用
 */
class SimpleUIExample : public ui::Application
{
public:
    // 使用父类构造函数
    using Application::Application;

protected:
    void setupUI() override
    {
        auto& registry = utils::Registry::getInstance();
        auto rootEntity = getRootEntity();

        // 创建一个简单的按钮
        auto button = registry.create();

        // 设置位置和大小
        registry.emplace<components::Position>(button, ImVec2{100.0f, 100.0f});
        registry.emplace<components::Size>(button, ImVec2{200.0f, 50.0f});

        // 设置背景和边框
        auto& bg = registry.emplace<components::Background>(button);
        bg.enabled = true;
        bg.color = ImVec4{0.2f, 0.4f, 0.8f, 1.0f};
        bg.borderRadius = 5.0f;

        auto& border = registry.emplace<components::Border>(button);
        border.enabled = true;
        border.color = ImVec4{1.0f, 1.0f, 1.0f, 1.0f};
        border.thickness = 2.0f;
        border.borderRadius = 5.0f;

        // 添加文本
        auto& text = registry.emplace<components::Text>(button);
        text.content = "Click Me!";
        text.color = ImVec4{1.0f, 1.0f, 1.0f, 1.0f};
        text.alignment = components::Alignment::CENTER;

        // 设置点击回调
        auto& clickable = registry.emplace<components::Clickable>(button);
        clickable.onClick = [](entt::entity entity)
        {
            auto& reg = utils::Registry::getInstance();
            auto& txt = reg.get<components::Text>(entity);
            txt.content = "Clicked!";
        };

        // 设置层级关系
        auto& hierarchy = registry.emplace<components::Hierarchy>(button);
        hierarchy.parent = rootEntity;

        // 将按钮添加到根实体的子列表
        auto& rootHierarchy = registry.get<components::Hierarchy>(rootEntity);
        rootHierarchy.children.push_back(button);

        // 设置可见标记
        registry.emplace<components::VisibleTag>(button);
        registry.emplace<components::ButtonTag>(button);
    }
};

} // namespace ui::examples
