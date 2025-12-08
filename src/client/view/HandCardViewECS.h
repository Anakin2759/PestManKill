/**
 * ************************************************************************
 *
 * @file HandCardViewECS.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief 手牌视图组件 - ECS版本
 * 显示单张手牌，支持选中状态
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
#include <entt/entt.hpp>
#include <string>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

namespace ui
{

/**
 * @brief 手牌视图组件 - ECS版本
 */
class HandCardViewECS
{
public:
    explicit HandCardViewECS(entt::registry& registry)
        : m_registry(registry), m_factory(registry), m_container(entt::null), m_selected(false)
    {
        createView();
    }

    ~HandCardViewECS() = default;

    // ===================== 卡牌信息设置 =====================
    void setCardName(const std::string& name)
    {
        m_cardName = name;
        if (m_registry.valid(m_cardNameLabel))
        {
            auto& label = m_registry.get<Label>(m_cardNameLabel);
            label.text = name;
        }
    }

    void setCardSuit(const std::string& suit)
    {
        m_cardSuit = suit;
        updateSuitLabel();
    }

    void setCardRank(const std::string& rank)
    {
        m_cardRank = rank;
        updateSuitLabel();
    }

    void setCardImage(const unsigned char* data, size_t size)
    {
        if (m_registry.valid(m_cardImage) && m_registry.all_of<Image>(m_cardImage))
        {
            auto& image = m_registry.get<Image>(m_cardImage);
            image.textureData = std::vector<unsigned char>(data, data + size);
            image.dataSize = size;
        }
    }

    // ===================== 选中状态 =====================
    void setSelected(bool selected)
    {
        m_selected = selected;
        updateSelectionStyle();

        // 通过全局事件分发器发布事件
        utils::Dispatcher::getInstance().enqueue<ui::events::CardSelectionChanged>(
            ui::events::CardSelectionChanged{m_container, m_selected, m_cardName});
    }

    void toggleSelect() { setSelected(!m_selected); }

    [[nodiscard]] bool isSelected() const { return m_selected; }

    [[nodiscard]] entt::entity getContainer() const { return m_container; }
    [[nodiscard]] const std::string& getCardName() const { return m_cardName; }
    [[nodiscard]] const std::string& getCardSuit() const { return m_cardSuit; }
    [[nodiscard]] const std::string& getCardRank() const { return m_cardRank; }

private:
    void createView()
    {
        // 创建主容器 - 竖向卡牌
        m_container = m_factory.createContainer({0.0F, 0.0F}, {95.0F, 135.0F}, ImVec4(0.15F, 0.17F, 0.20F, 0.95F));
        m_registry.emplace<VerticalLayout>(m_container, 4.0F);
        m_registry.emplace<Padding>(m_container, 6.0F, 6.0F, 6.0F, 6.0F);
        m_registry.emplace<Rounded>(m_container, 6.0F);
        m_registry.emplace<Clickable>(m_container);

        // 添加点击事件监听
        auto& clickable = m_registry.get<Clickable>(m_container);
        clickable.onClick = [this]() { toggleSelect(); };

        // 左上角花色和点数
        m_suitLabel = m_factory.createLabel({0.0F, 0.0F}, {0.0F, 0.0F}, "", 12.0F, ImVec4(1.0F, 1.0F, 1.0F, 1.0F));
        UIHelper::addChild(m_registry, m_container, m_suitLabel);

        // 卡牌图案区域
        m_cardImage = m_factory.createImage({0.0F, 0.0F}, {83.0F, 90.0F}, nullptr, 0);
        auto& imgBg = m_registry.get<Background>(m_cardImage);
        imgBg.enabled = true;
        imgBg.color = ImVec4(0.2F, 0.22F, 0.25F, 0.9F);
        UIHelper::addChild(m_registry, m_container, m_cardImage, 1.0F);

        // 卡牌名称
        m_cardNameLabel =
            m_factory.createLabel({0.0F, 0.0F}, {0.0F, 0.0F}, "卡牌", 12.0F, ImVec4(0.9F, 0.9F, 0.9F, 1.0F));
        UIHelper::addChild(m_registry, m_container, m_cardNameLabel);
    }

    void updateSuitLabel()
    {
        if (m_registry.valid(m_suitLabel))
        {
            auto& label = m_registry.get<Label>(m_suitLabel);
            label.text = m_cardSuit + m_cardRank;

            // 红桃/方片用红色，黑桃/梅花用黑色
            if (m_cardSuit == "♥" || m_cardSuit == "♦")
            {
                label.color = ImVec4(1.0F, 0.2F, 0.2F, 1.0F);
            }
            else
            {
                label.color = ImVec4(0.2F, 0.2F, 0.2F, 1.0F);
            }
        }
    }

    void updateSelectionStyle()
    {
        if (m_registry.valid(m_container) && m_registry.all_of<Background>(m_container))
        {
            auto& bg = m_registry.get<Background>(m_container);
            if (m_selected)
            {
                bg.color = ImVec4(0.3F, 0.4F, 0.6F, 0.95F); // 选中时高亮
            }
            else
            {
                bg.color = ImVec4(0.15F, 0.17F, 0.20F, 0.95F); // 正常颜色
            }
        }
    }

    entt::registry& m_registry;
    UIFactory m_factory;

    entt::entity m_container;
    entt::entity m_cardImage;
    entt::entity m_suitLabel;
    entt::entity m_cardNameLabel;

    std::string m_cardName;
    std::string m_cardSuit;
    std::string m_cardRank;
    bool m_selected;
};

} // namespace ui

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
