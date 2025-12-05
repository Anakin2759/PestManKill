/**
 * ************************************************************************
 *
 * @file OtherPlayerViewECS.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief 其他玩家视图组件 - ECS版本
 * 显示其他玩家的角色信息、状态、手牌数量、装备区
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
#include "EquipmentAreaECS.h"
#include <entt/entt.hpp>
#include <string>
#include <memory>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

namespace ui
{

/**
 * @brief 其他玩家视图组件 - ECS版本
 */
class OtherPlayerViewECS
{
public:
    explicit OtherPlayerViewECS(entt::registry& registry)
        : m_registry(registry), m_factory(registry), m_container(entt::null), m_handCardCount(0)
    {
        createView();
    }

    ~OtherPlayerViewECS() = default;

    // ===================== 设置玩家信息 =====================
    void setPlayerName(const std::string& name)
    {
        m_playerName = name;
        if (m_registry.valid(m_playerLabel))
        {
            auto& label = m_registry.get<Label>(m_playerLabel);
            label.text = name;
        }
    }

    void setOnlineStatus(bool online)
    {
        m_isOnline = online;
        if (m_registry.valid(m_statusLabel))
        {
            auto& label = m_registry.get<Label>(m_statusLabel);
            label.text = online ? "在线" : "离线";
            label.color = online ? ImVec4(0.2F, 0.8F, 0.2F, 1.0F) : ImVec4(0.8F, 0.2F, 0.2F, 1.0F);
        }
    }

    void setHp(int current, int max)
    {
        m_currentHp = current;
        m_maxHp = max;
        if (m_registry.valid(m_hpLabel))
        {
            auto& label = m_registry.get<Label>(m_hpLabel);
            label.text = "HP: " + std::to_string(current) + "/" + std::to_string(max);
        }
    }

    void setHandCardCount(int count)
    {
        m_handCardCount = count;
        if (m_registry.valid(m_handCardLabel))
        {
            auto& label = m_registry.get<Label>(m_handCardLabel);
            label.text = "手牌: " + std::to_string(count);
        }
    }

    void setIdentity(const std::string& identity)
    {
        m_identity = identity;
        updateInfoList();
    }

    void setFaction(const std::string& faction)
    {
        m_faction = faction;
        updateInfoList();
    }

    void setCharacterName(const std::string& name)
    {
        m_characterName = name;
        updateInfoList();
    }

    [[nodiscard]] entt::entity getContainer() const { return m_container; }
    [[nodiscard]] EquipmentAreaECS* getEquipmentArea() { return m_equipmentArea.get(); }

private:
    void createView()
    {
        // 创建主容器
        m_container = m_factory.createContainer({0.0F, 0.0F}, {220.0F, 380.0F}, ImVec4(0.12F, 0.14F, 0.17F, 0.95F));
        m_registry.emplace<VerticalLayout>(m_container, 8.0F);
        m_registry.emplace<Padding>(m_container, 10.0F, 10.0F, 10.0F, 10.0F);
        m_registry.emplace<Rounded>(m_container, 8.0F);

        // 顶部水平容器 - 包含信息列表和玩家状态
        auto topContainer = m_factory.createContainer({0.0F, 0.0F}, {0.0F, 0.0F}, ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
        m_registry.emplace<HorizontalLayout>(topContainer, 8.0F);
        UIHelper::addChild(m_registry, m_container, topContainer);

        // 左上角信息列表
        m_infoListContainer = m_factory.createContainer({0.0F, 0.0F}, {0.0F, 0.0F}, ImVec4(0.0F, 0.0F, 0.0F, 0.6F));
        m_registry.emplace<VerticalLayout>(m_infoListContainer, 2.0F);
        m_registry.emplace<Padding>(m_infoListContainer, 4.0F, 4.0F, 4.0F, 4.0F);
        UIHelper::addChild(m_registry, topContainer, m_infoListContainer, 1.0F);

        // 右上角玩家状态
        auto statusContainer = m_factory.createContainer({0.0F, 0.0F}, {0.0F, 0.0F}, ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
        m_registry.emplace<VerticalLayout>(statusContainer, 4.0F);
        UIHelper::addChild(m_registry, topContainer, statusContainer, 1.0F);

        m_playerLabel =
            m_factory.createLabel({0.0F, 0.0F}, {0.0F, 0.0F}, "玩家", 14.0F, ImVec4(0.9F, 0.9F, 0.9F, 1.0F));
        UIHelper::addChild(m_registry, statusContainer, m_playerLabel);

        m_statusLabel =
            m_factory.createLabel({0.0F, 0.0F}, {0.0F, 0.0F}, "离线", 14.0F, ImVec4(0.8F, 0.2F, 0.2F, 1.0F));
        UIHelper::addChild(m_registry, statusContainer, m_statusLabel);

        m_hpLabel = m_factory.createLabel({0.0F, 0.0F}, {0.0F, 0.0F}, "HP: 0/0", 14.0F, ImVec4(1.0F, 0.3F, 0.3F, 1.0F));
        UIHelper::addChild(m_registry, statusContainer, m_hpLabel);

        // 手牌数量
        m_handCardLabel =
            m_factory.createLabel({0.0F, 0.0F}, {0.0F, 0.0F}, "手牌: 0", 16.0F, ImVec4(1.0F, 0.8F, 0.4F, 1.0F));
        UIHelper::addChild(m_registry, m_container, m_handCardLabel);

        // 装备区
        m_equipmentArea = std::make_unique<EquipmentAreaECS>(m_registry);
        UIHelper::addChild(m_registry, m_container, m_equipmentArea->getContainer());
    }

    void updateInfoList()
    {
        // 清空现有子项
        if (m_registry.valid(m_infoListContainer) && m_registry.all_of<Children>(m_infoListContainer))
        {
            auto& children = m_registry.get<Children>(m_infoListContainer);
            for (auto child : children.entities)
            {
                if (m_registry.valid(child))
                {
                    m_registry.destroy(child);
                }
            }
            children.entities.clear();
        }

        // 身份
        if (!m_identity.empty())
        {
            auto label = m_factory.createLabel(
                {0.0F, 0.0F}, {0.0F, 0.0F}, "身份: " + m_identity, 12.0F, ImVec4(1.0F, 0.9F, 0.3F, 1.0F));
            UIHelper::addChild(m_registry, m_infoListContainer, label);
        }

        // 阵营
        if (!m_faction.empty())
        {
            auto label = m_factory.createLabel(
                {0.0F, 0.0F}, {0.0F, 0.0F}, "阵营: " + m_faction, 12.0F, ImVec4(0.3F, 0.8F, 1.0F, 1.0F));
            UIHelper::addChild(m_registry, m_infoListContainer, label);
        }

        // 角色名
        if (!m_characterName.empty())
        {
            auto label = m_factory.createLabel(
                {0.0F, 0.0F}, {0.0F, 0.0F}, "角色: " + m_characterName, 12.0F, ImVec4(1.0F, 0.6F, 0.3F, 1.0F));
            UIHelper::addChild(m_registry, m_infoListContainer, label);
        }
    }

    entt::registry& m_registry;
    UIFactory m_factory;

    entt::entity m_container;
    entt::entity m_infoListContainer;
    entt::entity m_playerLabel;
    entt::entity m_statusLabel;
    entt::entity m_hpLabel;
    entt::entity m_handCardLabel;

    std::unique_ptr<EquipmentAreaECS> m_equipmentArea;

    std::string m_playerName;
    std::string m_identity;
    std::string m_faction;
    std::string m_characterName;
    bool m_isOnline{false};
    int m_currentHp{0};
    int m_maxHp{0};
    int m_handCardCount;
};

} // namespace ui

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
