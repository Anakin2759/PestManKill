/**
 * ************************************************************************
 *
 * @file EquipmentArea.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief 装备区视图组件
    * 显示玩家的装备信息，如武器、防具、坐骑等
    一个长方形区域，宽大于高度
    卡面（背景）是装备卡牌图案
    有一个横向的label
    显示装备区标题（如"装备区"）
    有一个横向的label
    显示当前装备的武器名称和图标
    有一个横向的label
    显示当前装备的防具名称和图标
    有一个横向的listArea 居中对齐 无滚动条
    显示当前装备的坐骑名称和图标 两件装备 进攻坐骑 防御坐骑

    - 提供一个接口用于装备 装备牌
    - 提供一个接口用于 卸下装备
    - 提供一个接口用于 获取当前装备信息
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
#include "src/client/ui/Image.h"
#include "src/client/ui/ListArea.h"
#include <memory>
#include <string>
#include <optional>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

/**
 * @brief 装备区视图组件
 * 显示玩家的装备信息：武器、防具、进攻坐骑、防御坐骑
 */
class EquipmentArea : public ui::Widget
{
public:
    enum class EquipmentType : uint8_t
    {
        WEAPON,          // 武器
        ARMOR,           // 防具
        OFFENSIVE_HORSE, // 进攻坐骑（+1马）
        DEFENSIVE_HORSE  // 防御坐骑（-1马）
    };

    struct Equipment
    {
        std::string name;
        std::string iconPath;
        EquipmentType type;
    };

    EquipmentArea() { setupUI(); }

    ~EquipmentArea() override = default;

    // ===================== 装备管理 =====================
    void equipItem(EquipmentType type, const std::string& name, const std::string& iconPath = "")
    {
        Equipment equip{name, iconPath, type};

        switch (type)
        {
            case EquipmentType::WEAPON:
                m_weapon = equip;
                updateWeaponDisplay();
                break;
            case EquipmentType::ARMOR:
                m_armor = equip;
                updateArmorDisplay();
                break;
            case EquipmentType::OFFENSIVE_HORSE:
                m_offensiveHorse = equip;
                updateHorseDisplay();
                break;
            case EquipmentType::DEFENSIVE_HORSE:
                m_defensiveHorse = equip;
                updateHorseDisplay();
                break;
        }
    }

    // 便捷方法
    template <typename... Args>
    void equipWeapon(Args&&... args)
    {
        equipItem(EquipmentType::WEAPON, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void equipArmor(Args&&... args)
    {
        equipItem(EquipmentType::ARMOR, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void equipOffensiveHorse(Args&&... args)
    {
        equipItem(EquipmentType::OFFENSIVE_HORSE, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void equipDefensiveHorse(Args&&... args)
    {
        equipItem(EquipmentType::DEFENSIVE_HORSE, std::forward<Args>(args)...);
    }

    void unequipWeapon() { unequipItem(EquipmentType::WEAPON); }
    void unequipArmor() { unequipItem(EquipmentType::ARMOR); }
    void unequipOffensiveHorse() { unequipItem(EquipmentType::OFFENSIVE_HORSE); }
    void unequipDefensiveHorse() { unequipItem(EquipmentType::DEFENSIVE_HORSE); }

    void unequipItem(EquipmentType type)
    {
        switch (type)
        {
            case EquipmentType::WEAPON:
                m_weapon.reset();
                updateWeaponDisplay();
                break;
            case EquipmentType::ARMOR:
                m_armor.reset();
                updateArmorDisplay();
                break;
            case EquipmentType::OFFENSIVE_HORSE:
                m_offensiveHorse.reset();
                updateHorseDisplay();
                break;
            case EquipmentType::DEFENSIVE_HORSE:
                m_defensiveHorse.reset();
                updateHorseDisplay();
                break;
        }
    }

    void clearAllEquipment()
    {
        m_weapon.reset();
        m_armor.reset();
        m_offensiveHorse.reset();
        m_defensiveHorse.reset();
        updateAllDisplays();
    }

    // ===================== 获取装备信息 =====================
    [[nodiscard]] const std::optional<Equipment>& getWeapon() const { return m_weapon; }
    [[nodiscard]] const std::optional<Equipment>& getArmor() const { return m_armor; }
    [[nodiscard]] const std::optional<Equipment>& getOffensiveHorse() const { return m_offensiveHorse; }
    [[nodiscard]] const std::optional<Equipment>& getDefensiveHorse() const { return m_defensiveHorse; }

    [[nodiscard]] bool hasWeapon() const { return m_weapon.has_value(); }
    [[nodiscard]] bool hasArmor() const { return m_armor.has_value(); }
    [[nodiscard]] bool hasOffensiveHorse() const { return m_offensiveHorse.has_value(); }
    [[nodiscard]] bool hasDefensiveHorse() const { return m_defensiveHorse.has_value(); }

protected:
    ImVec2 calculateSize() override
    {
        if (isFixedSize())
        {
            return Widget::calculateSize();
        }
        return m_mainLayout->calculateSize();
    }

    void onRender(const ImVec2& position, const ImVec2& size) override
    {
        // 渲染背景
        if (isBackgroundEnabled())
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 bottomRight(position.x + size.x, position.y + size.y);
            drawList->AddRectFilled(position, bottomRight, ImGui::ColorConvertFloat4ToU32(getBackgroundColor()), 6.0F);
        }

        // 渲染主布局
        m_mainLayout->render(position, size);
    }

private:
    void setupUI()
    {
        setBackgroundEnabled(true);
        setBackgroundColor(ImVec4(0.12F, 0.14F, 0.17F, 0.9F));

        m_mainLayout = std::make_shared<ui::VBoxLayout>();
        m_mainLayout->setMargins(8, 8, 8, 8);
        m_mainLayout->setSpacing(6);

        // 标题
        auto titleLabel = std::make_shared<ui::Label>("装备区");
        titleLabel->setTextColor(ImVec4(1.0F, 0.9F, 0.3F, 1.0F));
        titleLabel->setBackgroundEnabled(false);
        m_mainLayout->addWidget(titleLabel, 0);

        // 武器区域
        m_weaponLayout = createEquipmentRow();
        m_weaponIcon = std::static_pointer_cast<ui::Image>(m_weaponLayout->getItems()[0].widget);
        m_weaponLabel = std::static_pointer_cast<ui::Label>(m_weaponLayout->getItems()[1].widget);
        m_mainLayout->addWidget(m_weaponLayout, 0);

        // 防具区域
        m_armorLayout = createEquipmentRow();
        m_armorIcon = std::static_pointer_cast<ui::Image>(m_armorLayout->getItems()[0].widget);
        m_armorLabel = std::static_pointer_cast<ui::Label>(m_armorLayout->getItems()[1].widget);
        m_mainLayout->addWidget(m_armorLayout, 0);

        // 坐骑区域（横向ListArea）
        m_horseArea = std::make_shared<ui::ListArea>(ui::ListDirection::HORIZONTAL);
        m_horseArea->setScrollbarEnabled(false);
        m_horseArea->setAlignment(ui::ListAlignment::CENTER);
        m_horseArea->setItemSpacing(8.0F);

        // 进攻坐骑
        m_offensiveHorseLayout = createEquipmentRow();
        m_offensiveHorseIcon = std::static_pointer_cast<ui::Image>(m_offensiveHorseLayout->getItems()[0].widget);
        m_offensiveHorseLabel = std::static_pointer_cast<ui::Label>(m_offensiveHorseLayout->getItems()[1].widget);
        m_horseArea->addWidget(m_offensiveHorseLayout);

        // 防御坐骑
        m_defensiveHorseLayout = createEquipmentRow();
        m_defensiveHorseIcon = std::static_pointer_cast<ui::Image>(m_defensiveHorseLayout->getItems()[0].widget);
        m_defensiveHorseLabel = std::static_pointer_cast<ui::Label>(m_defensiveHorseLayout->getItems()[1].widget);
        m_horseArea->addWidget(m_defensiveHorseLayout);

        m_mainLayout->addWidget(m_horseArea, 0);

        updateAllDisplays();
        addChild(m_mainLayout);
    }

    std::shared_ptr<ui::HBoxLayout> createEquipmentRow()
    {
        auto row = std::make_shared<ui::HBoxLayout>();
        row->setSpacing(6);

        auto icon = std::make_shared<ui::Image>();
        icon->setFixedSize(32, 32);
        icon->setBackgroundEnabled(true);
        icon->setBackgroundColor(ImVec4(0.2F, 0.2F, 0.3F, 0.8F));

        auto label = std::make_shared<ui::Label>("未装备");
        label->setTextColor(ImVec4(0.6F, 0.6F, 0.6F, 1.0F));
        label->setBackgroundEnabled(false);

        row->addWidget(icon, 0);
        row->addWidget(label, 1);

        return row;
    }

    void updateWeaponDisplay()
    {
        if (m_weapon.has_value())
        {
            m_weaponLabel->setText("武器: " + m_weapon->name);
            m_weaponLabel->setTextColor(ImVec4(1.0F, 0.4F, 0.4F, 1.0F));
            if (!m_weapon->iconPath.empty())
            {
                m_weaponIcon->setImagePath(m_weapon->iconPath);
            }
        }
        else
        {
            m_weaponLabel->setText("武器: 未装备");
            m_weaponLabel->setTextColor(ImVec4(0.6F, 0.6F, 0.6F, 1.0F));
            m_weaponIcon->clear();
        }
    }

    void updateArmorDisplay()
    {
        if (m_armor.has_value())
        {
            m_armorLabel->setText("防具: " + m_armor->name);
            m_armorLabel->setTextColor(ImVec4(0.4F, 0.8F, 1.0F, 1.0F));
            if (!m_armor->iconPath.empty())
            {
                m_armorIcon->setImagePath(m_armor->iconPath);
            }
        }
        else
        {
            m_armorLabel->setText("防具: 未装备");
            m_armorLabel->setTextColor(ImVec4(0.6F, 0.6F, 0.6F, 1.0F));
            m_armorIcon->clear();
        }
    }

    void updateHorseDisplay()
    {
        // 进攻坐骑
        if (m_offensiveHorse.has_value())
        {
            m_offensiveHorseLabel->setText("+1马: " + m_offensiveHorse->name);
            m_offensiveHorseLabel->setTextColor(ImVec4(1.0F, 0.7F, 0.3F, 1.0F));
            if (!m_offensiveHorse->iconPath.empty())
            {
                m_offensiveHorseIcon->setImagePath(m_offensiveHorse->iconPath);
            }
        }
        else
        {
            m_offensiveHorseLabel->setText("+1马: 未装备");
            m_offensiveHorseLabel->setTextColor(ImVec4(0.6F, 0.6F, 0.6F, 1.0F));
            m_offensiveHorseIcon->clear();
        }

        // 防御坐骑
        if (m_defensiveHorse.has_value())
        {
            m_defensiveHorseLabel->setText("-1马: " + m_defensiveHorse->name);
            m_defensiveHorseLabel->setTextColor(ImVec4(0.3F, 1.0F, 0.7F, 1.0F));
            if (!m_defensiveHorse->iconPath.empty())
            {
                m_defensiveHorseIcon->setImagePath(m_defensiveHorse->iconPath);
            }
        }
        else
        {
            m_defensiveHorseLabel->setText("-1马: 未装备");
            m_defensiveHorseLabel->setTextColor(ImVec4(0.6F, 0.6F, 0.6F, 1.0F));
            m_defensiveHorseIcon->clear();
        }
    }

    void updateAllDisplays()
    {
        updateWeaponDisplay();
        updateArmorDisplay();
        updateHorseDisplay();
    }

    // 成员变量
    std::shared_ptr<ui::VBoxLayout> m_mainLayout;
    std::shared_ptr<ui::ListArea> m_horseArea;

    // 武器
    std::shared_ptr<ui::HBoxLayout> m_weaponLayout;
    std::shared_ptr<ui::Image> m_weaponIcon;
    std::shared_ptr<ui::Label> m_weaponLabel;

    // 防具
    std::shared_ptr<ui::HBoxLayout> m_armorLayout;
    std::shared_ptr<ui::Image> m_armorIcon;
    std::shared_ptr<ui::Label> m_armorLabel;

    // 进攻坐骑
    std::shared_ptr<ui::HBoxLayout> m_offensiveHorseLayout;
    std::shared_ptr<ui::Image> m_offensiveHorseIcon;
    std::shared_ptr<ui::Label> m_offensiveHorseLabel;

    // 防御坐骑
    std::shared_ptr<ui::HBoxLayout> m_defensiveHorseLayout;
    std::shared_ptr<ui::Image> m_defensiveHorseIcon;
    std::shared_ptr<ui::Label> m_defensiveHorseLabel;

    // 装备数据
    std::optional<Equipment> m_weapon;
    std::optional<Equipment> m_armor;
    std::optional<Equipment> m_offensiveHorse;
    std::optional<Equipment> m_defensiveHorse;
};

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
