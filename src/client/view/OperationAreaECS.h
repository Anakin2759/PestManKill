/**
 * ************************************************************************
 *
 * @file OperationAreaECS.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief 操作区视图组件 - ECS版本
 * 显示操作按钮和手牌区域
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
 * @brief 操作区视图组件 - ECS版本
 */
class OperationAreaECS
{
public:
    explicit OperationAreaECS(entt::registry& registry)
        : m_registry(registry), m_factory(registry), m_container(entt::null)
    {
        createView();
    }

    ~OperationAreaECS() = default;

    // ===================== 手牌管理 =====================
    void addHandCard(std::shared_ptr<HandCardViewECS> card)
    {
        if (!card)
        {
            return;
        }

        m_handCards.push_back(card);
        UIHelper::addChild(m_registry, m_handCardContainer, card->getContainer());
    }

    void removeHandCard(std::shared_ptr<HandCardViewECS> card)
    {
        if (!card)
        {
            return;
        }

        auto it = std::find(m_handCards.begin(), m_handCards.end(), card);
        if (it != m_handCards.end())
        {
            // 从容器中移除
            if (m_registry.valid(m_handCardContainer) && m_registry.all_of<Children>(m_handCardContainer))
            {
                auto& children = m_registry.get<Children>(m_handCardContainer);
                auto childIt = std::find(children.entities.begin(), children.entities.end(), card->getContainer());
                if (childIt != children.entities.end())
                {
                    children.entities.erase(childIt);
                }
            }

            m_handCards.erase(it);
        }
    }

    void clearHandCards()
    {
        // 清空所有手牌
        if (m_registry.valid(m_handCardContainer) && m_registry.all_of<Children>(m_handCardContainer))
        {
            auto& children = m_registry.get<Children>(m_handCardContainer);
            children.entities.clear();
        }
        m_handCards.clear();
    }

    [[nodiscard]] size_t getHandCardCount() const { return m_handCards.size(); }

    [[nodiscard]] const std::vector<std::shared_ptr<HandCardViewECS>>& getHandCards() const { return m_handCards; }

    std::shared_ptr<HandCardViewECS> getSelectedCard() const
    {
        for (const auto& card : m_handCards)
        {
            if (card->isSelected())
            {
                return card;
            }
        }
        return nullptr;
    }

    std::vector<std::shared_ptr<HandCardViewECS>> getSelectedCards() const
    {
        std::vector<std::shared_ptr<HandCardViewECS>> selected;
        for (const auto& card : m_handCards)
        {
            if (card->isSelected())
            {
                selected.push_back(card);
            }
        }
        return selected;
    }

    // ===================== 按钮回调设置（已弃用，使用全局事件分发器） =====================
    // 保留这些方法以保持兼容性，但内部不再使用回调

    [[nodiscard]] entt::entity getContainer() const { return m_container; }
    void setUseCardEnabled(bool enabled)
    {
        if (m_registry.valid(m_useCardBtn) && m_registry.all_of<Button>(m_useCardBtn))
        {
            auto& btn = m_registry.get<Button>(m_useCardBtn);
            btn.enabled = enabled;
        }
    }

    void setCancelEnabled(bool enabled)
    {
        if (m_registry.valid(m_cancelBtn) && m_registry.all_of<Button>(m_cancelBtn))
        {
            auto& btn = m_registry.get<Button>(m_cancelBtn);
            btn.enabled = enabled;
        }
    }

    void setEndTurnEnabled(bool enabled)
    {
        if (m_registry.valid(m_endTurnBtn) && m_registry.all_of<Button>(m_endTurnBtn))
        {
            auto& btn = m_registry.get<Button>(m_endTurnBtn);
            btn.enabled = enabled;
        }
    }

    [[nodiscard]] entt::entity getContainer() const { return m_container; }

private:
    void createView()
    {
        // 创建主容器 - 垂直布局
        m_container = m_factory.createContainer({0.0F, 0.0F}, {0.0F, 0.0F}, ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
        m_registry.emplace<VerticalLayout>(m_container, 10.0F);
        m_registry.emplace<Padding>(m_container, 10.0F, 10.0F, 10.0F, 10.0F);

        // 上方按钮区域
        auto buttonContainer = m_factory.createContainer({0.0F, 0.0F}, {0.0F, 0.0F}, ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
        m_registry.emplace<HorizontalLayout>(buttonContainer, 10.0F);
        UIHelper::addChild(m_registry, m_container, buttonContainer);

        // 使用卡牌按钮
        m_useCardBtn = m_factory.createButton(
            {0.0F, 0.0F},
            {120.0F, 40.0F},
            "使用卡牌",
            [this]()
            {
                auto selected = getSelectedCard();
                if (selected)
                {
                    utils::Dispatcher::getInstance().enqueue<ui::events::UseCardEvent>(
                        ui::events::UseCardEvent{selected->getContainer(), selected->getCardName()});
                }
            });
        UIHelper::addChild(m_registry, buttonContainer, m_useCardBtn);

        // 取消按钮
        m_cancelBtn =
            m_factory.createButton({0.0F, 0.0F},
                                   {120.0F, 40.0F},
                                   "取消",
                                   [this]()
                                   {
                                       utils::Dispatcher::getInstance().enqueue<ui::events::CancelOperationEvent>(
                                           ui::events::CancelOperationEvent{});
                                   });
        UIHelper::addChild(m_registry, buttonContainer, m_cancelBtn);

        // 结束回合按钮
        m_endTurnBtn = m_factory.createButton(
            {0.0F, 0.0F},
            {120.0F, 40.0F},
            "结束回合",
            [this]()
            { utils::Dispatcher::getInstance().enqueue<ui::events::EndTurnEvent>(ui::events::EndTurnEvent{}); });
        UIHelper::addChild(m_registry, buttonContainer, m_endTurnBtn);

        // 下方手牌区域
        m_handCardContainer = m_factory.createContainer({0.0F, 0.0F}, {0.0F, 0.0F}, ImVec4(0.12F, 0.14F, 0.16F, 0.75F));
        m_registry.emplace<HorizontalLayout>(m_handCardContainer, 8.0F);
        m_registry.emplace<Padding>(m_handCardContainer, 10.0F, 10.0F, 10.0F, 10.0F);
        UIHelper::addChild(m_registry, m_container, m_handCardContainer, 1.0F);
    }

    entt::registry& m_registry;
    UIFactory m_factory;

    entt::entity m_container;
    entt::entity m_useCardBtn;
    entt::entity m_cancelBtn;
    entt::entity m_endTurnBtn;
    entt::entity m_handCardContainer;

    std::vector<std::shared_ptr<HandCardViewECS>> m_handCards;
};

} // namespace ui

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
