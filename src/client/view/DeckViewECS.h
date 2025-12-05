/**
 * ************************************************************************
 *
 * @file DeckViewECS.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief 卡组视图组件 - ECS版本
 * 显示牌堆和弃牌堆的数量
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include "src/client/model/UiSystem.h"
#include "src/client/model/UIFactory.h"
#include "src/client/model/UIHelper.h"
#include "src/client/components/UIComponents.h"
#include <entt/entt.hpp>
#include <string>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

namespace ui
{

/**
 * @brief 卡组视图组件 - ECS版本
 * 显示牌堆和弃牌堆信息
 */
class DeckViewECS
{
public:
    explicit DeckViewECS(entt::registry& registry)
        : m_registry(registry), m_factory(registry), m_container(entt::null), m_deckCount(0), m_discardCount(0)
    {
        createView();
    }

    ~DeckViewECS() = default;

    // ===================== 设置数量 =====================
    void setDeckCount(int count)
    {
        m_deckCount = count;
        if (m_registry.valid(m_deckLabel))
        {
            auto& label = m_registry.get<Label>(m_deckLabel);
            label.text = "牌堆: " + std::to_string(count);
        }
    }

    void setDiscardCount(int count)
    {
        m_discardCount = count;
        if (m_registry.valid(m_discardLabel))
        {
            auto& label = m_registry.get<Label>(m_discardLabel);
            label.text = "弃牌堆: " + std::to_string(count);
        }
    }

    [[nodiscard]] int getDeckCount() const { return m_deckCount; }
    [[nodiscard]] int getDiscardCount() const { return m_discardCount; }
    [[nodiscard]] entt::entity getContainer() const { return m_container; }

private:
    void createView()
    {
        // 创建主容器
        m_container = m_factory.createContainer({0.0F, 0.0F}, {0.0F, 0.0F}, ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
        m_registry.emplace<VerticalLayout>(m_container, 10.0F);
        m_registry.emplace<Padding>(m_container, 8.0F, 8.0F, 8.0F, 8.0F);

        // 牌堆图标和标签
        auto deckRow = m_factory.createContainer({0.0F, 0.0F}, {0.0F, 0.0F}, ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
        m_registry.emplace<HorizontalLayout>(deckRow, 8.0F);
        UIHelper::addChild(m_registry, m_container, deckRow);

        // 牌堆图标
        m_deckIcon = m_factory.createImage({0.0F, 0.0F}, {40.0F, 40.0F}, nullptr, 0);
        auto& deckIconBg = m_registry.get<Background>(m_deckIcon);
        deckIconBg.enabled = true;
        deckIconBg.color = ImVec4(0.2F, 0.3F, 0.5F, 0.8F);
        UIHelper::addChild(m_registry, deckRow, m_deckIcon);

        // 牌堆标签
        m_deckLabel =
            m_factory.createLabel({0.0F, 0.0F}, {0.0F, 0.0F}, "牌堆: 0", 16.0F, ImVec4(0.9F, 0.9F, 0.9F, 1.0F));
        UIHelper::addChild(m_registry, deckRow, m_deckLabel, 1.0F);

        // 弃牌堆图标和标签
        auto discardRow = m_factory.createContainer({0.0F, 0.0F}, {0.0F, 0.0F}, ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
        m_registry.emplace<HorizontalLayout>(discardRow, 8.0F);
        UIHelper::addChild(m_registry, m_container, discardRow);

        // 弃牌堆图标
        m_discardIcon = m_factory.createImage({0.0F, 0.0F}, {40.0F, 40.0F}, nullptr, 0);
        auto& discardIconBg = m_registry.get<Background>(m_discardIcon);
        discardIconBg.enabled = true;
        discardIconBg.color = ImVec4(0.5F, 0.3F, 0.2F, 0.8F);
        UIHelper::addChild(m_registry, discardRow, m_discardIcon);

        // 弃牌堆标签
        m_discardLabel =
            m_factory.createLabel({0.0F, 0.0F}, {0.0F, 0.0F}, "弃牌堆: 0", 16.0F, ImVec4(0.9F, 0.9F, 0.9F, 1.0F));
        UIHelper::addChild(m_registry, discardRow, m_discardLabel, 1.0F);
    }

    entt::registry& m_registry;
    UIFactory m_factory;

    entt::entity m_container;
    entt::entity m_deckIcon;
    entt::entity m_deckLabel;
    entt::entity m_discardIcon;
    entt::entity m_discardLabel;

    int m_deckCount;
    int m_discardCount;
};

} // namespace ui

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
