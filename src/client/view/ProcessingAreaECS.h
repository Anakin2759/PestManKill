/**
 * ************************************************************************
 *
 * @file ProcessingAreaECS.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief 处理区视图组件 - ECS版本
 * 显示当前回合打出的卡牌
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
#include "src/client/events/UIEvents.h"
#include "src/client/utils/Dispatcher.h"
#include "HandCardViewECS.h"
#include <entt/entt.hpp>
#include <string>
#include <vector>
#include <memory>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

namespace ui
{

/**
 * @brief 处理区视图组件 - ECS版本
 */
class ProcessingAreaECS
{
public:
    explicit ProcessingAreaECS(entt::registry& registry)
        : m_registry(registry), m_factory(registry), m_container(entt::null)
    {
        createView();
    }

    ~ProcessingAreaECS() = default;

    // ===================== 卡牌管理 =====================
    void addCard(std::shared_ptr<HandCardViewECS> card)
    {
        if (!card)
        {
            return;
        }

        m_cards.push_back(card);
        UIHelper::addChild(m_registry, m_cardContainer, card->getContainer());

        // 通过全局事件分发器发布事件
        utils::Dispatcher::getInstance().enqueue<ui::events::CardMovedToProcessing>(
            ui::events::CardMovedToProcessing{card->getContainer(), card->getCardName()});
    }

    void removeCard(std::shared_ptr<HandCardViewECS> card)
    {
        if (!card)
        {
            return;
        }

        auto it = std::find(m_cards.begin(), m_cards.end(), card);
        if (it != m_cards.end())
        {
            // 从容器中移除
            if (m_registry.valid(m_cardContainer) && m_registry.all_of<Children>(m_cardContainer))
            {
                auto& children = m_registry.get<Children>(m_cardContainer);
                auto childIt = std::find(children.entities.begin(), children.entities.end(), card->getContainer());
                if (childIt != children.entities.end())
                {
                    children.entities.erase(childIt);
                }
            }

            m_cards.erase(it);

            // 通过全局事件分发器发布事件
            utils::Dispatcher::getInstance().enqueue<ui::events::CardRemovedFromProcessing>(
                ui::events::CardRemovedFromProcessing{card->getContainer(), card->getCardName()});
        }
    }

    void clearAllCards()
    {
        // 通过全局事件分发器发布清空前事件
        std::vector<entt::entity> cardEntities;
        for (const auto& card : m_cards)
        {
            cardEntities.push_back(card->getContainer());
        }
        utils::Dispatcher::getInstance().enqueue<ui::events::ProcessingAreaBeforeClear>(
            ui::events::ProcessingAreaBeforeClear{cardEntities});

        // 清空所有卡牌
        if (m_registry.valid(m_cardContainer) && m_registry.all_of<Children>(m_cardContainer))
        {
            auto& children = m_registry.get<Children>(m_cardContainer);
            children.entities.clear();
        }

        m_cards.clear();

        // 通过全局事件分发器发布清空后事件
        utils::Dispatcher::getInstance().enqueue<ui::events::ProcessingAreaAfterClear>(
            ui::events::ProcessingAreaAfterClear{});
    }

    [[nodiscard]] size_t getCardCount() const { return m_cards.size(); }

    [[nodiscard]] const std::vector<std::shared_ptr<HandCardViewECS>>& getCards() const { return m_cards; }

    [[nodiscard]] entt::entity getContainer() const { return m_container; }

private:
    void createView()
    {
        // 创建主容器
        m_container = m_factory.createContainer({0.0F, 0.0F}, {0.0F, 0.0F}, ImVec4(0.12F, 0.14F, 0.18F, 0.55F));
        m_registry.emplace<VerticalLayout>(m_container, 10.0F);
        m_registry.emplace<Padding>(m_container, 15.0F, 15.0F, 15.0F, 15.0F);

        // 标题
        auto title = m_factory.createLabel({0.0F, 0.0F}, {0.0F, 0.0F}, "处理区", 18.0F, ImVec4(1.0F, 0.9F, 0.5F, 1.0F));
        UIHelper::addChild(m_registry, m_container, title);

        // 卡牌容器 - 横向布局
        m_cardContainer = m_factory.createContainer({0.0F, 0.0F}, {0.0F, 0.0F}, ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
        m_registry.emplace<HorizontalLayout>(m_cardContainer, 10.0F);
        UIHelper::addChild(m_registry, m_container, m_cardContainer, 1.0F);
    }

    entt::registry& m_registry;
    UIFactory m_factory;

    entt::entity m_container;
    entt::entity m_cardContainer;

    std::vector<std::shared_ptr<HandCardViewECS>> m_cards;
};

} // namespace ui

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
