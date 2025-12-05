/**
 * ************************************************************************
 *
 * @file EquipmentAreaECS.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief 装备区视图组件 - ECS版本
 * 显示玩家的装备信息：武器、防具、进攻坐骑、防御坐骑
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
#include <optional>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

namespace ui
{

/**
 * @brief 装备区视图组件 - ECS版本
 */
class EquipmentAreaECS
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

    explicit EquipmentAreaECS(entt::registry& registry)
        : m_registry(registry), m_factory(registry), m_container(entt::null)
    {
        createView();
    }

    ~EquipmentAreaECS() = default;

    // ===================== 装备管理 =====================
    void equipWeapon(const std::string& name, const std::string& iconPath = "")
    {
        m_weapon = Equipment{name, iconPath, EquipmentType::WEAPON};
        updateWeaponDisplay();
    }

    void equipArmor(const std::string& name, const std::string& iconPath = "")
    {
        m_armor = Equipment{name, iconPath, EquipmentType::ARMOR};
        updateArmorDisplay();
    }

    void equipOffensiveHorse(const std::string& name, const std::string& iconPath = "")
    {
        m_offensiveHorse = Equipment{name, iconPath, EquipmentType::OFFENSIVE_HORSE};
        updateHorsesDisplay();
    }

    void equipDefensiveHorse(const std::string& name, const std::string& iconPath = "")
    {
        m_defensiveHorse = Equipment{name, iconPath, EquipmentType::DEFENSIVE_HORSE};
        updateHorsesDisplay();
    }

    void unequipWeapon()
    {
        m_weapon.reset();
        updateWeaponDisplay();
    }

    void unequipArmor()
    {
        m_armor.reset();
        updateArmorDisplay();
    }

    void unequipOffensiveHorse()
    {
        m_offensiveHorse.reset();
        updateHorsesDisplay();
    }

    void unequipDefensiveHorse()
    {
        m_defensiveHorse.reset();
        updateHorsesDisplay();
    }

    void clearAll()
    {
        m_weapon.reset();
        m_armor.reset();
        m_offensiveHorse.reset();
        m_defensiveHorse.reset();
        updateWeaponDisplay();
        updateArmorDisplay();
        updateHorsesDisplay();
    }

    [[nodiscard]] entt::entity getContainer() const { return m_container; }
    [[nodiscard]] const std::optional<Equipment>& getWeapon() const { return m_weapon; }
    [[nodiscard]] const std::optional<Equipment>& getArmor() const { return m_armor; }
    [[nodiscard]] const std::optional<Equipment>& getOffensiveHorse() const { return m_offensiveHorse; }
    [[nodiscard]] const std::optional<Equipment>& getDefensiveHorse() const { return m_defensiveHorse; }

private:
    void createView()
    {
        // 创建主容器 - 横向布局
        m_container = m_factory.createContainer({0.0F, 0.0F}, {0.0F, 0.0F}, ImVec4(0.14F, 0.16F, 0.19F, 0.85F));
        m_registry.emplace<VerticalLayout>(m_container, 8.0F);
        m_registry.emplace<Padding>(m_container, 10.0F, 10.0F, 10.0F, 10.0F);

        // 标题
        auto title = m_factory.createLabel({0.0F, 0.0F}, {0.0F, 0.0F}, "装备区", 16.0F, ImVec4(1.0F, 0.8F, 0.4F, 1.0F));
        UIHelper::addChild(m_registry, m_container, title);

        // 武器行
        m_weaponRow = m_factory.createContainer({0.0F, 0.0F}, {0.0F, 0.0F}, ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
        m_registry.emplace<HorizontalLayout>(m_weaponRow, 8.0F);
        UIHelper::addChild(m_registry, m_container, m_weaponRow);

        m_weaponLabel =
            m_factory.createLabel({0.0F, 0.0F}, {0.0F, 0.0F}, "武器: 无", 14.0F, ImVec4(0.9F, 0.9F, 0.9F, 1.0F));
        UIHelper::addChild(m_registry, m_weaponRow, m_weaponLabel, 1.0F);

        // 防具行
        m_armorRow = m_factory.createContainer({0.0F, 0.0F}, {0.0F, 0.0F}, ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
        m_registry.emplace<HorizontalLayout>(m_armorRow, 8.0F);
        UIHelper::addChild(m_registry, m_container, m_armorRow);

        m_armorLabel =
            m_factory.createLabel({0.0F, 0.0F}, {0.0F, 0.0F}, "防具: 无", 14.0F, ImVec4(0.9F, 0.9F, 0.9F, 1.0F));
        UIHelper::addChild(m_registry, m_armorRow, m_armorLabel, 1.0F);

        // 坐骑行
        m_horsesRow = m_factory.createContainer({0.0F, 0.0F}, {0.0F, 0.0F}, ImVec4(0.0F, 0.0F, 0.0F, 0.0F));
        m_registry.emplace<HorizontalLayout>(m_horsesRow, 8.0F);
        UIHelper::addChild(m_registry, m_container, m_horsesRow);

        m_offensiveHorseLabel =
            m_factory.createLabel({0.0F, 0.0F}, {0.0F, 0.0F}, "+1马: 无", 14.0F, ImVec4(0.9F, 0.9F, 0.9F, 1.0F));
        UIHelper::addChild(m_registry, m_horsesRow, m_offensiveHorseLabel, 1.0F);

        m_defensiveHorseLabel =
            m_factory.createLabel({0.0F, 0.0F}, {0.0F, 0.0F}, "-1马: 无", 14.0F, ImVec4(0.9F, 0.9F, 0.9F, 1.0F));
        UIHelper::addChild(m_registry, m_horsesRow, m_defensiveHorseLabel, 1.0F);
    }

    void updateWeaponDisplay()
    {
        if (m_registry.valid(m_weaponLabel))
        {
            auto& label = m_registry.get<Label>(m_weaponLabel);
            label.text = m_weapon.has_value() ? "武器: " + m_weapon->name : "武器: 无";
        }
    }

    void updateArmorDisplay()
    {
        if (m_registry.valid(m_armorLabel))
        {
            auto& label = m_registry.get<Label>(m_armorLabel);
            label.text = m_armor.has_value() ? "防具: " + m_armor->name : "防具: 无";
        }
    }

    void updateHorsesDisplay()
    {
        if (m_registry.valid(m_offensiveHorseLabel))
        {
            auto& label = m_registry.get<Label>(m_offensiveHorseLabel);
            label.text = m_offensiveHorse.has_value() ? "+1马: " + m_offensiveHorse->name : "+1马: 无";
        }

        if (m_registry.valid(m_defensiveHorseLabel))
        {
            auto& label = m_registry.get<Label>(m_defensiveHorseLabel);
            label.text = m_defensiveHorse.has_value() ? "-1马: " + m_defensiveHorse->name : "-1马: 无";
        }
    }

    entt::registry& m_registry;
    UIFactory m_factory;

    entt::entity m_container;
    entt::entity m_weaponRow;
    entt::entity m_armorRow;
    entt::entity m_horsesRow;
    entt::entity m_weaponLabel;
    entt::entity m_armorLabel;
    entt::entity m_offensiveHorseLabel;
    entt::entity m_defensiveHorseLabel;

    std::optional<Equipment> m_weapon;
    std::optional<Equipment> m_armor;
    std::optional<Equipment> m_offensiveHorse;
    std::optional<Equipment> m_defensiveHorse;
};

} // namespace ui

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
