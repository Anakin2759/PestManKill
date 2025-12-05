/**
 * ************************************************************************
 *
 * @file CharacterViewECS.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief 角色视图组件 - ECS版本
 * 显示角色卡牌信息，包括身份、阵营、角色名、技能等
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
#include <vector>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

namespace ui
{

/**
 * @brief 角色视图组件 - ECS版本
 * 宽 < 高的长方形区域，显示角色信息和技能
 */
class CharacterViewECS
{
public:
    struct SkillInfo
    {
        std::string name;        // 技能名称
        std::string description; // 技能描述
    };

    explicit CharacterViewECS(entt::registry& registry)
        : m_registry(registry), m_factory(registry), m_container(entt::null)
    {
        createView();
    }

    ~CharacterViewECS() = default;

    // ===================== 设置角色信息 =====================
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

    void setHp(int current, int max)
    {
        m_currentHp = current;
        m_maxHp = max;
        updateInfoList();
    }

    void setCharacterName(const std::string& name)
    {
        m_characterName = name;
        updateInfoList();
    }

    void setPlayerName(const std::string& name)
    {
        m_playerName = name;
        if (m_registry.valid(m_playerLabel))
        {
            auto& label = m_registry.get<Label>(m_playerLabel);
            label.text = "玩家: " + name;
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

    void setCharacterImage(const unsigned char* data, size_t size)
    {
        if (m_registry.valid(m_characterImage) && m_registry.all_of<Image>(m_characterImage))
        {
            auto& image = m_registry.get<Image>(m_characterImage);
            image.textureData = std::vector<unsigned char>(data, data + size);
            image.dataSize = size;
        }
    }

    // ===================== 技能管理 =====================
    void addSkill(const std::string& skillName, const std::string& description)
    {
        m_skills.push_back({skillName, description});
        updateSkillButtons();
    }

    void addSkill(const SkillInfo& skill)
    {
        m_skills.push_back(skill);
        updateSkillButtons();
    }

    void clearSkills()
    {
        m_skills.clear();
        updateSkillButtons();
    }

    void setSkills(const std::vector<SkillInfo>& skills)
    {
        m_skills = skills;
        updateSkillButtons();
    }

    [[nodiscard]] const std::vector<SkillInfo>& getSkills() const { return m_skills; }

    // ===================== 获取信息 =====================
    [[nodiscard]] entt::entity getContainer() const { return m_container; }
    [[nodiscard]] const std::string& getIdentity() const { return m_identity; }
    [[nodiscard]] const std::string& getFaction() const { return m_faction; }
    [[nodiscard]] const std::string& getCharacterName() const { return m_characterName; }
    [[nodiscard]] const std::string& getPlayerName() const { return m_playerName; }
    [[nodiscard]] bool isOnline() const { return m_isOnline; }

private:
    void createView()
    {
        // 创建主容器 - 深色背景
        m_container = m_factory.createContainer({0.0F, 0.0F},
                                                {200.0F, 320.0F}, // 宽 < 高
                                                ImVec4(0.12F, 0.14F, 0.17F, 0.95F));
        m_registry.emplace<VerticalLayout>(m_container, 8.0F);
        m_registry.emplace<Padding>(m_container, 8.0F, 8.0F, 8.0F, 8.0F);
        m_registry.emplace<Rounded>(m_container, 8.0F);

        // 角色卡面图片区域
        m_characterImage = m_factory.createImage({0.0F, 0.0F}, {184.0F, 200.0F}, nullptr, 0);
        auto& imgBg = m_registry.get<Background>(m_characterImage);
        imgBg.enabled = true;
        imgBg.color = ImVec4(0.16F, 0.18F, 0.22F, 0.95F);
        UIHelper::addChild(m_registry, m_container, m_characterImage);

        // 左上角信息列表容器（叠加在图片上的效果需要特殊处理）
        m_infoListContainer = m_factory.createContainer({0.0F, 0.0F}, {0.0F, 0.0F}, ImVec4(0.0F, 0.0F, 0.0F, 0.6F));
        m_registry.emplace<VerticalLayout>(m_infoListContainer, 2.0F);
        m_registry.emplace<Padding>(m_infoListContainer, 4.0F, 4.0F, 4.0F, 4.0F);
        UIHelper::addChild(m_registry, m_container, m_infoListContainer);

        // 玩家信息区域
        auto playerInfoContainer =
            m_factory.createContainer({0.0F, 0.0F}, {0.0F, 0.0F}, ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
        m_registry.emplace<HorizontalLayout>(playerInfoContainer, 8.0F);
        UIHelper::addChild(m_registry, m_container, playerInfoContainer);

        m_playerLabel =
            m_factory.createLabel({0.0F, 0.0F}, {0.0F, 0.0F}, "玩家: 未知", 14.0F, ImVec4(0.9F, 0.9F, 0.9F, 1.0F));
        UIHelper::addChild(m_registry, playerInfoContainer, m_playerLabel, 1.0F);

        m_statusLabel =
            m_factory.createLabel({0.0F, 0.0F}, {0.0F, 0.0F}, "离线", 14.0F, ImVec4(0.8F, 0.2F, 0.2F, 1.0F));
        UIHelper::addChild(m_registry, playerInfoContainer, m_statusLabel);

        // 技能按钮区域
        m_skillContainer = m_factory.createContainer({0.0F, 0.0F}, {0.0F, 0.0F}, ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
        m_registry.emplace<VerticalLayout>(m_skillContainer, 4.0F);
        UIHelper::addChild(m_registry, m_container, m_skillContainer);
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

        // 体力值
        if (m_maxHp > 0)
        {
            auto label = m_factory.createLabel({0.0F, 0.0F},
                                               {0.0F, 0.0F},
                                               "HP: " + std::to_string(m_currentHp) + "/" + std::to_string(m_maxHp),
                                               12.0F,
                                               ImVec4(1.0F, 0.3F, 0.3F, 1.0F));
            UIHelper::addChild(m_registry, m_infoListContainer, label);
        }
    }

    void updateSkillButtons()
    {
        // 清空现有技能按钮
        if (m_registry.valid(m_skillContainer) && m_registry.all_of<Children>(m_skillContainer))
        {
            auto& children = m_registry.get<Children>(m_skillContainer);
            for (auto child : children.entities)
            {
                if (m_registry.valid(child))
                {
                    m_registry.destroy(child);
                }
            }
            children.entities.clear();
        }

        // 创建技能按钮
        for (const auto& skill : m_skills)
        {
            auto btn = m_factory.createButton(
                {0.0F, 0.0F}, {160.0F, 30.0F}, skill.name, [this, skill]() { showSkillDescription(skill); });
            UIHelper::addChild(m_registry, m_skillContainer, btn);
        }
    }

    void showSkillDescription(const SkillInfo& skill)
    {
        // TODO: 实现技能描述对话框
        // 这里需要一个模态对话框系统，暂时使用日志输出
        utils::LOG_INFO("技能: {} - {}", skill.name, skill.description);
    }

    entt::registry& m_registry;
    UIFactory m_factory;

    entt::entity m_container;
    entt::entity m_characterImage;
    entt::entity m_infoListContainer;
    entt::entity m_playerLabel;
    entt::entity m_statusLabel;
    entt::entity m_skillContainer;

    std::string m_identity;
    std::string m_faction;
    std::string m_characterName;
    std::string m_playerName;
    bool m_isOnline{false};
    int m_currentHp{0};
    int m_maxHp{0};
    std::vector<SkillInfo> m_skills;
};

} // namespace ui

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
