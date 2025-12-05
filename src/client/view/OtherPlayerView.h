/**
 * ************************************************************************
 *
 * @file OtherPlayerView.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-04
 * @version 0.1
 * @brief 其他玩家视图组件
    * 显示其他玩家的信息，如头像、名称、状态等
    一个长方形区域，宽小于高度
    卡面（背景）是角色卡牌图案
    左上角有一个小的竖着的无滚动条的listArea
    显示身份名称
    显示阵营名称
    显示角色名称



    右上角有一个ListArea
    有一个横向的label
    显示玩家名称
    有一个横向的label
    显示玩家状态（在线、离线）
    有一个横向的label
    显示玩家体力值（血量）

    下方是一个大区域
    label 显示玩家手牌数量
    显示玩家装备区（武器、防具、坐骑等）




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
#include "src/client/ui/ListArea.h"
#include "src/client/ui/Image.h"
#include "EquipmentArea.h"
#include <memory>
#include <string>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

/**
 * @brief 其他玩家视图组件
 * 显示其他玩家的角色信息、状态、手牌数量、装备区
 */
class OtherPlayerView : public ui::Widget
{
public:
    OtherPlayerView()
    {
        setupUI();
        setFixedSize(220, 380);
    }

    ~OtherPlayerView() override = default;

    // ===================== 设置玩家信息 =====================
    void setPlayerName(const std::string& name)
    {
        m_playerName = name;
        m_playerLabel->setText(name);
    }

    void setOnlineStatus(bool online)
    {
        m_isOnline = online;
        m_statusLabel->setText(online ? "在线" : "离线");
        m_statusLabel->setTextColor(online ? ImVec4(0.2F, 0.8F, 0.2F, 1.0F) : ImVec4(0.8F, 0.2F, 0.2F, 1.0F));
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

    void setCharacterImage(const std::string& imagePath) { m_characterImage->setImagePath(imagePath); }

    void setHp(int current, int max)
    {
        m_currentHp = current;
        m_maxHp = max;
        updateHpDisplay();
    }

    void setHandCardCount(int count)
    {
        m_handCardCount = count;
        m_handCardLabel->setText("手牌: " + std::to_string(count));
    }

    // ===================== 装备管理 =====================
    void equipItem(EquipmentArea::EquipmentType type, const std::string& name, const std::string& iconPath = "")
    {
        m_equipmentArea->equipItem(type, name, iconPath);
    }

    void unequipItem(EquipmentArea::EquipmentType type) { m_equipmentArea->unequipItem(type); }

    [[nodiscard]] std::shared_ptr<EquipmentArea> getEquipmentArea() const { return m_equipmentArea; }

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

        m_mainLayout->render(position, size);
    }

private:
    void setupUI()
    {
        setBackgroundEnabled(true);
        setBackgroundColor(ImVec4(0.12F, 0.14F, 0.17F, 0.95F));

        m_mainLayout = std::make_shared<ui::VBoxLayout>();
        m_mainLayout->setMargins(8, 8, 8, 8);
        m_mainLayout->setSpacing(6);

        // 顶部：角色卡面 + 信息叠加层
        setupCardArea();

        // 中部：玩家信息
        setupPlayerInfo();

        // 下部：手牌数量和装备区
        setupBottomArea();

        addChild(m_mainLayout);
    }

    void setupCardArea()
    {
        // 角色卡面
        m_characterImage = std::make_shared<ui::Image>();
        m_characterImage->setFixedSize(204, 140);
        m_characterImage->setBackgroundEnabled(true);
        m_characterImage->setBackgroundColor(ImVec4(0.16F, 0.18F, 0.22F, 0.95F));
        m_mainLayout->addWidget(m_characterImage, 0);

        // 左上角信息列表（身份/阵营/角色）
        m_infoList = std::make_shared<ui::ListArea>(ui::ListDirection::VERTICAL);
        m_infoList->setScrollbarEnabled(false);
        m_infoList->setItemSpacing(2.0F);
        m_infoList->setMargins(4.0F);
        m_infoList->setBackgroundEnabled(true);
        m_infoList->setBackgroundColor(ImVec4(0.0F, 0.0F, 0.0F, 0.6F));
        m_infoList->setFixedSize(80, 60);

        updateInfoList();
    }

    void setupPlayerInfo()
    {
        auto infoArea = std::make_shared<ui::VBoxLayout>();
        infoArea->setSpacing(4);

        // 玩家名称
        m_playerLabel = std::make_shared<ui::Label>("未知玩家");
        m_playerLabel->setTextColor(ImVec4(0.9F, 0.9F, 0.9F, 1.0F));
        m_playerLabel->setBackgroundEnabled(false);
        infoArea->addWidget(m_playerLabel, 0);

        // 状态 + HP
        auto statusRow = std::make_shared<ui::HBoxLayout>();
        statusRow->setSpacing(6);

        m_statusLabel = std::make_shared<ui::Label>("离线");
        m_statusLabel->setTextColor(ImVec4(0.8F, 0.2F, 0.2F, 1.0F));
        m_statusLabel->setBackgroundEnabled(false);
        statusRow->addWidget(m_statusLabel, 0);

        m_hpLabel = std::make_shared<ui::Label>("HP: 0/0");
        m_hpLabel->setTextColor(ImVec4(1.0F, 0.3F, 0.3F, 1.0F));
        m_hpLabel->setBackgroundEnabled(false);
        statusRow->addWidget(m_hpLabel, 1);

        infoArea->addWidget(statusRow, 0);
        m_mainLayout->addWidget(infoArea, 0);
    }

    void setupBottomArea()
    {
        // 手牌数量
        m_handCardLabel = std::make_shared<ui::Label>("手牌: 0");
        m_handCardLabel->setTextColor(ImVec4(0.7F, 0.9F, 1.0F, 1.0F));
        m_handCardLabel->setBackgroundEnabled(false);
        m_mainLayout->addWidget(m_handCardLabel, 0);

        // 装备区
        m_equipmentArea = std::make_shared<EquipmentArea>();
        m_mainLayout->addWidget(m_equipmentArea, 1);
    }

    void updateInfoList()
    {
        m_infoList->clear();

        if (!m_identity.empty())
        {
            auto label = std::make_shared<ui::Label>(m_identity);
            label->setBackgroundEnabled(false);
            label->setTextColor(ImVec4(1.0F, 0.9F, 0.3F, 1.0F));
            m_infoList->addWidget(label);
        }

        if (!m_faction.empty())
        {
            auto label = std::make_shared<ui::Label>(m_faction);
            label->setBackgroundEnabled(false);
            label->setTextColor(ImVec4(0.3F, 0.8F, 1.0F, 1.0F));
            m_infoList->addWidget(label);
        }

        if (!m_characterName.empty())
        {
            auto label = std::make_shared<ui::Label>(m_characterName);
            label->setBackgroundEnabled(false);
            label->setTextColor(ImVec4(1.0F, 0.6F, 0.3F, 1.0F));
            m_infoList->addWidget(label);
        }
    }

    void updateHpDisplay()
    {
        std::string hpText = "HP: " + std::to_string(m_currentHp) + "/" + std::to_string(m_maxHp);

        // 根据血量百分比设置颜色
        float hpPercent = m_maxHp > 0 ? static_cast<float>(m_currentHp) / static_cast<float>(m_maxHp) : 0.0F;
        ImVec4 color;
        if (hpPercent > 0.6F)
        {
            color = ImVec4(0.2F, 1.0F, 0.3F, 1.0F); // 绿色
        }
        else if (hpPercent > 0.3F)
        {
            color = ImVec4(1.0F, 0.9F, 0.2F, 1.0F); // 黄色
        }
        else
        {
            color = ImVec4(1.0F, 0.2F, 0.2F, 1.0F); // 红色
        }

        m_hpLabel->setText(hpText);
        m_hpLabel->setTextColor(color);
    }

    // 成员变量
    std::shared_ptr<ui::VBoxLayout> m_mainLayout;
    std::shared_ptr<ui::Image> m_characterImage;
    std::shared_ptr<ui::ListArea> m_infoList;
    std::shared_ptr<ui::Label> m_playerLabel;
    std::shared_ptr<ui::Label> m_statusLabel;
    std::shared_ptr<ui::Label> m_hpLabel;
    std::shared_ptr<ui::Label> m_handCardLabel;
    std::shared_ptr<EquipmentArea> m_equipmentArea;

    std::string m_playerName;
    std::string m_identity;
    std::string m_faction;
    std::string m_characterName;
    bool m_isOnline{false};
    int m_currentHp{0};
    int m_maxHp{0};
    int m_handCardCount{0};
};

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)