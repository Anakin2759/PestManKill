/**
 * ************************************************************************
 *
 * @file CharacterView.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-04
 * @version 0.1
 * @brief 角色视图组件
    显示角色卡牌信息
    包含角色名称、技能描述等信息

    一个长方形区域，宽小于高度
    卡面（背景）是角色卡牌图案
    下方有一个或多个button
    显示技能名称，点击后弹出技能描述
    左上角包含一个竖向的无滚动 `ListArea`，显示身份、阵营、角色名
    右侧包含玩家名称、在线状态标签与操作按钮区域

    - 提供方法设置角色信息（身份、阵营、角色名、玩家名、在线状态等）
    - 提供方法设置角色卡牌图案（也可没有）
    - 提供方法添加技能按钮，点击弹出技能描述对话框
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include "src/client/ui/Widget.h"
#include "src/client/ui/Layout.h"
#include "src/client/ui/Label.h"
#include "src/client/ui/Button.h"
#include "src/client/ui/ListArea.h"
#include "src/client/ui/Image.h"
#include "src/client/ui/Dialog.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

/**
 * @brief 角色视图组件
 * 显示角色卡牌信息，包括身份、阵营、角色名、技能等
 */
class CharacterView : public ui::Widget
{
public:
    struct SkillInfo
    {
        std::string name;        // 技能名称
        std::string description; // 技能描述
    };

    CharacterView()
    {
        setupUI();
        setFixedSize(200, 320); // 宽 < 高
    }

    ~CharacterView() override = default;

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
        m_playerLabel->setText("玩家: " + name);
    }

    void setOnlineStatus(bool online)
    {
        m_isOnline = online;
        m_statusLabel->setText(online ? "在线" : "离线");
        m_statusLabel->setTextColor(online ? ImVec4(0.2F, 0.8F, 0.2F, 1.0F) : ImVec4(0.8F, 0.2F, 0.2F, 1.0F));
    }

    void setCharacterImage(const std::string& imagePath) { m_characterImage->setImagePath(imagePath); }

    void setCharacterImage(const unsigned char* data, size_t size) { m_characterImage->loadFromMemory(data, size); }

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

    // ===================== 获取角色信息 =====================
    [[nodiscard]] const std::string& getIdentity() const { return m_identity; }
    [[nodiscard]] const std::string& getFaction() const { return m_faction; }
    [[nodiscard]] const std::string& getCharacterName() const { return m_characterName; }
    [[nodiscard]] const std::string& getPlayerName() const { return m_playerName; }
    [[nodiscard]] bool isOnline() const { return m_isOnline; }

protected:
    void onRender(const ImVec2& position, const ImVec2& size) override
    {
        // 渲染背景
        if (isBackgroundEnabled())
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 bottomRight(position.x + size.x, position.y + size.y);
            drawList->AddRectFilled(position, bottomRight, ImGui::ColorConvertFloat4ToU32(getBackgroundColor()), 8.0F);
        }

        // 渲染主布局
        m_mainLayout->render(position, size);
    }

private:
    void setupUI()
    {
        setBackgroundEnabled(true);
        setBackgroundColor(ImVec4(0.12F, 0.14F, 0.17F, 0.95F));

        m_mainLayout = std::make_shared<ui::VBoxLayout>();
        m_mainLayout->setMargins(8, 8, 8, 8);
        m_mainLayout->setSpacing(8);

        // 上部：角色卡面区域
        setupCardArea();

        // 中部：玩家信息区域
        setupPlayerInfo();

        // 下部：技能按钮区域
        setupSkillArea();

        addChild(m_mainLayout);
    }

    void setupCardArea()
    {
        auto cardArea = std::make_shared<ui::VBoxLayout>();
        cardArea->setSpacing(4);

        // 角色卡面图片
        m_characterImage = std::make_shared<ui::Image>();
        m_characterImage->setFixedSize(184, 200);
        m_characterImage->setBackgroundEnabled(true);
        m_characterImage->setBackgroundColor(ImVec4(0.16F, 0.18F, 0.22F, 0.95F));

        cardArea->addWidget(m_characterImage);

        // 左上角信息列表（覆盖在图片上）
        m_infoList = std::make_shared<ui::ListArea>(ui::ListDirection::VERTICAL);
        m_infoList->setScrollbarEnabled(false);
        m_infoList->setItemSpacing(2.0F);
        m_infoList->setMargins(4.0F);
        m_infoList->setBackgroundEnabled(true);
        m_infoList->setBackgroundColor(ImVec4(0.0F, 0.0F, 0.0F, 0.6F));

        updateInfoList();

        // 使用绝对定位将信息列表放在左上角
        // 这里需要手动管理位置
        cardArea->addWidget(m_infoList);

        m_mainLayout->addWidget(cardArea, 0);
    }

    void setupPlayerInfo()
    {
        auto playerInfo = std::make_shared<ui::HBoxLayout>();
        playerInfo->setSpacing(8);

        m_playerLabel = std::make_shared<ui::Label>("玩家: 未知");
        m_playerLabel->setTextColor(ImVec4(0.9F, 0.9F, 0.9F, 1.0F));

        m_statusLabel = std::make_shared<ui::Label>("离线");
        m_statusLabel->setTextColor(ImVec4(0.8F, 0.2F, 0.2F, 1.0F));

        playerInfo->addWidget(m_playerLabel, 1);
        playerInfo->addWidget(m_statusLabel, 0);

        m_mainLayout->addWidget(playerInfo, 0);
    }

    void setupSkillArea()
    {
        m_skillArea = std::make_shared<ui::ListArea>(ui::ListDirection::VERTICAL);
        m_skillArea->setScrollbarEnabled(false);
        m_skillArea->setItemSpacing(4.0F);
        m_skillArea->setAlignment(ui::ListAlignment::CENTER);

        m_mainLayout->addWidget(m_skillArea, 0);
    }

    void updateInfoList()
    {
        m_infoList->clear();

        if (!m_identity.empty())
        {
            auto label = std::make_shared<ui::Label>("身份: " + m_identity);
            label->setBackgroundEnabled(false);
            label->setTextColor(ImVec4(1.0F, 0.9F, 0.3F, 1.0F));
            m_infoList->addWidget(label);
        }

        if (!m_faction.empty())
        {
            auto label = std::make_shared<ui::Label>("阵营: " + m_faction);
            label->setBackgroundEnabled(false);
            label->setTextColor(ImVec4(0.3F, 0.8F, 1.0F, 1.0F));
            m_infoList->addWidget(label);
        }

        if (!m_characterName.empty())
        {
            auto label = std::make_shared<ui::Label>("角色: " + m_characterName);
            label->setBackgroundEnabled(false);
            label->setTextColor(ImVec4(1.0F, 0.6F, 0.3F, 1.0F));
            m_infoList->addWidget(label);
        }
    }

    void updateSkillButtons()
    {
        m_skillArea->clear();

        for (const auto& skill : m_skills)
        {
            auto btn = std::make_shared<ui::Button>(skill.name);
            btn->setFixedSize(160, 30);

            // 点击技能按钮显示技能描述对话框
            btn->setOnClick([this, skill]() { showSkillDescription(skill); });

            m_skillArea->addWidget(btn);
        }
    }

    void showSkillDescription(const SkillInfo& skill)
    {
        // 创建并显示技能描述对话框
        auto dialog = std::make_shared<ui::Dialog>("技能: " + skill.name);
        dialog->setFixedSize(400, 250);

        auto content = std::make_shared<ui::VBoxLayout>();
        content->setMargins(16, 16, 16, 16);
        content->setSpacing(12);

        // 技能名称
        auto nameLabel = std::make_shared<ui::Label>(skill.name);
        nameLabel->setTextColor(ImVec4(1.0F, 0.9F, 0.3F, 1.0F));
        content->addWidget(nameLabel);

        // 技能描述
        auto descLabel = std::make_shared<ui::Label>(skill.description);
        descLabel->setTextColor(ImVec4(0.9F, 0.9F, 0.9F, 1.0F));
        content->addWidget(descLabel, 1);

        // 关闭按钮
        auto closeBtn = std::make_shared<ui::Button>("关闭");
        closeBtn->setFixedSize(100, 35);
        closeBtn->setOnClick([dialog]() { dialog->close(); });

        auto btnLayout = std::make_shared<ui::HBoxLayout>();
        btnLayout->addStretch(1);
        btnLayout->addWidget(closeBtn, 0);
        content->addWidget(btnLayout, 0);

        dialog->setContent(content);
        dialog->show();
    }

    // 成员变量
    std::shared_ptr<ui::VBoxLayout> m_mainLayout;
    std::shared_ptr<ui::Image> m_characterImage;
    std::shared_ptr<ui::ListArea> m_infoList;
    std::shared_ptr<ui::Label> m_playerLabel;
    std::shared_ptr<ui::Label> m_statusLabel;
    std::shared_ptr<ui::ListArea> m_skillArea;

    std::string m_identity;
    std::string m_faction;
    std::string m_characterName;
    std::string m_playerName;
    bool m_isOnline{false};
    int m_currentHp{0};
    int m_maxHp{0};
    std::vector<SkillInfo> m_skills;
};

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)