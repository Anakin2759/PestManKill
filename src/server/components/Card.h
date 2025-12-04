/**
 * ************************************************************************
 *
 * @file Card.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-11-19
 * @version 0.1
 * @brief
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <cstdint>
#include <string>
#include <functional>
#include <entt/entt.hpp>
#include "src/shared/common/Common.h"

struct MetaCardInfo
{
    std::string name;                // 牌名
    std::string description;         // 新增描述字段
    CardType type = CardType::BASIC; // 牌类型
};

struct CardCost
{
    uint32_t mana = 0;
    uint32_t energy = 0;
    uint32_t health = 0;
};
struct CardTarget
{
    bool needTarget = true;
    uint8_t maxTargets = 1;
    uint8_t minTargets = 1;
    uint8_t range = 0; // 0表示无限制

    std::function<bool(entt::entity)> filter; // 目标筛选规则
};

struct CardPointAndSuit
{
    uint8_t point = 0;               // 0表示无点数，1-13分别表示A到K
    SuitType suit = SuitType::JOKER; // JOKER表示无花色
};
struct CardEffect
{
    entt::delegate<void(entt::entity, std::span<entt::entity>, entt::registry&)> apply;
};

struct BasicCardTypeTag
{
    BasicCardType type;
};

struct StrategyCardTypeTag
{
    StrategyCardType type;
};

struct EquipCardTypeTag
{
    EquipCardType type;
};

inline entt::entity CreateCard(entt::registry& reg,
                               const MetaCardInfo& metaInfo,
                               const CardCost& cost,
                               const CardTarget& target,
                               const CardPointAndSuit& pointAndSuit,
                               const CardEffect& effect)
{
    auto ent = reg.create();
    reg.emplace<MetaCardInfo>(ent, metaInfo);
    reg.emplace<CardCost>(ent, cost);
    reg.emplace<CardTarget>(ent, target);
    reg.emplace<CardPointAndSuit>(ent, pointAndSuit);
    reg.emplace<CardEffect>(ent, effect);
    return ent;
}

inline entt::entity CreateBasicCard(entt::registry& reg,
                                    const MetaCardInfo& metaInfo,
                                    const CardCost& cost,
                                    const CardTarget& target,
                                    const CardPointAndSuit& pointAndSuit,
                                    const CardEffect& effect,
                                    BasicCardType basicType)
{
    auto ent = CreateCard(reg, metaInfo, cost, target, pointAndSuit, effect);
    reg.emplace<BasicCardTypeTag>(ent, BasicCardTypeTag{basicType});
    return ent;
}

inline entt::entity CreateStrategyCard(entt::registry& reg,
                                       const MetaCardInfo& metaInfo,
                                       const CardCost& cost,
                                       const CardTarget& target,
                                       const CardPointAndSuit& pointAndSuit,
                                       const CardEffect& effect,
                                       StrategyCardType strategyType)
{
    auto ent = CreateCard(reg, metaInfo, cost, target, pointAndSuit, effect);
    reg.emplace<StrategyCardTypeTag>(ent, StrategyCardTypeTag{strategyType});
    return ent;
}

inline entt::entity CreateEquipCard(entt::registry& reg,
                                    const MetaCardInfo& metaInfo,
                                    const CardCost& cost,
                                    const CardTarget& target,
                                    const CardPointAndSuit& pointAndSuit,
                                    const CardEffect& effect,
                                    EquipCardType equipType)
{
    auto ent = CreateCard(reg, metaInfo, cost, target, pointAndSuit, effect);
    reg.emplace<EquipCardTypeTag>(ent, EquipCardTypeTag{equipType});
    return ent;
}
/**
 * @brief 创建杀卡牌实体
 * @param reg 注册表
 * @param pointAndSuit 点数和花色
 * @return entt::entity 实体ID
 */
inline entt::entity CreateStrickCard(entt::registry& reg, const CardPointAndSuit& pointAndSuit)
{
    CardTarget target{
        .needTarget = true,
        .minTargets = 1,
        .maxTargets = 1,
        .range = 1,
    };
    CardCost cost{};
    CardEffect effect{.apply = [](entt::entity user, std::span<entt::entity> targets, entt::registry& reg)
                      {
                          for (auto target : targets)
                          {
                              // reg.trigger<events::CardUsed>(user, target, reg);
                          }
                      }};

    return CreateBasicCard(reg,
                           {.name = "杀", .description = "需要使用一张闪否则造成一点伤害", .type = CardType::BASIC},
                           cost,
                           target,
                           pointAndSuit,
                           effect,
                           BasicCardType::STRIKE);
}

/**
 * @brief 创建闪卡牌实体
 * @param reg 注册表
 * @param pointAndSuit 点数和花色
 * @return entt::entity 实体ID
 */
inline entt::entity CreateDodgeCard(entt::registry& reg, const CardPointAndSuit& pointAndSuit)
{
    CardTarget target{.needTarget = false, .minTargets = 0, .maxTargets = 0, .range = 0};
    MetaCardInfo metaInfo{.name = "闪", .description = "用于抵消一张杀的伤害", .type = CardType::BASIC};
    CardCost cost{};
    CardEffect effect{.apply = [](entt::entity user, std::span<entt::entity> targets, entt::registry& reg)
                      {
                          for (auto target : targets)
                          {
                              // reg.trigger<events::CardUsed>(user, target, reg);
                          }
                      }};
    return CreateBasicCard(reg, metaInfo, cost, target, pointAndSuit, effect, BasicCardType::DODGE);
}

inline entt::entity CreatePeachCard(entt::registry& reg,
                                    const MetaCardInfo& metaInfo,
                                    const CardCost& cost,
                                    const CardPointAndSuit& pointAndSuit,
                                    const CardEffect& effect)
{
    CardTarget target{};
    target.needTarget = true;
    target.minTargets = 1;
    target.maxTargets = 1;
    target.range = 0; // 无距离限制

    return CreateBasicCard(reg, metaInfo, cost, target, pointAndSuit, effect, BasicCardType::PEACH);
}

/**
 * @brief 创建酒卡牌实体
 * @param reg 注册表
 * @param pointAndSuit 点数和花色
 * @return entt::entity 实体ID
 */
inline entt::entity CreateAlcoholCard(entt::registry& reg, const CardPointAndSuit& pointAndSuit)
{
    CardTarget target{};
    target.needTarget = true;
    target.minTargets = 1;
    target.maxTargets = 1;
    target.range = 0; // 无距离限制

    MetaInfo metaInfo{.name = "酒",
                      .description = "回合内使用后，下一次受到的伤害-1（至少为1）,濒死状态下使用可回复1点体力",
                      .type = CardType::BASIC};
    CardCost cost{};
    CardEffect effect{.apply = [](entt::entity user, std::span<entt::entity> targets, entt::registry& reg)
                      {
                          for (auto target : targets)
                          {
                              // reg.trigger<events::CardUsed>(user, target, reg);
                          }
                      }};
    return CreateBasicCard(reg, metaInfo, cost, target, pointAndSuit, effect, BasicCardType::ALCOHOL);
}

/**
 * @brief 创建火攻卡牌实体
 * @param reg 注册表
 * @param pointAndSuit 点数和花色
 * @return entt::entity 实体ID
 */
inline entt::entity CreateFireAttackCard(entt::registry& reg, const CardPointAndSuit& pointAndSuit)
{
    CardTarget target{};
    target.needTarget = true;
    target.minTargets = 1;
    target.maxTargets = 1;
    target.range = 0;
    MetaInfo metaInfo{.name = "火攻",
                      .description = "对目标角色造成一点火焰伤害，目标角色可以使用一张闪避来抵消伤害",
                      .type = CardType::STRATEGY};
    CardCost cost{};
    CardEffect effect{.apply = [](entt::entity user, std::span<entt::entity> targets, entt::registry& reg)
                      {
                          for (auto target : targets)
                          {
                              // reg.trigger<events::CardUsed>(user, target, reg);
                          }
                      }};

    return CreateStrategyCard(reg, metaInfo, cost, target, pointAndSuit, effect, StrategyCardType::FIRE_ATTACK);
}

/**
 * @brief 创建决斗卡牌实体
 * @param reg 注册表
 * @param pointAndSuit 点数和花色
 * @return entt::entity 实体ID
 */
inline entt::entity CreateDuelCard(entt::registry& reg, const CardPointAndSuit& pointAndSuit)
{
    CardTarget target{
        .needTarget = true,
        .minTargets = 1,
        .maxTargets = 1,
        .range = 0xFF, // 无距离限制
    };
    MetaInfo metaInfo{.name = "决斗",
                      .description = "与你指定的角色进行决斗，双方轮流出杀，未能出杀的一方受到一点伤害",
                      .type = CardType::STRATEGY};
    CardCost cost{};
    CardEffect effect{.apply = [](entt::entity user, std::span<entt::entity> targets, entt::registry& reg)
                      {
                          for (auto target : targets)
                          {
                              // reg.trigger<events::CardUsed>(user, target, reg);
                          }
                      }};

    return CreateStrategyCard(reg, metaInfo, cost, target, pointAndSuit, effect, StrategyCardType::DUEL);
}