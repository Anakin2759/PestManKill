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
    std::string name;
    std::string description; // 新增描述字段
    CardType type = CardType::BASIC;
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

inline entt::entity CreateAttackCard(entt::registry& reg,
                                     const MetaCardInfo& metaInfo,
                                     const CardCost& cost,
                                     const CardPointAndSuit& pointAndSuit,
                                     const CardEffect& effect)
{
    CardTarget target{};
    target.needTarget = true;
    target.minTargets = 1;
    target.maxTargets = 1;
    target.range = 1; // 近战攻击

    return CreateBasicCard(reg, metaInfo, cost, target, pointAndSuit, effect, BasicCardType::STRIKE);
}

inline entt::entity CreateDodgeCard(entt::registry& reg,
                                    const MetaCardInfo& metaInfo,
                                    const CardCost& cost,
                                    const CardPointAndSuit& pointAndSuit,
                                    const CardEffect& effect)
{
    CardTarget target{};
    target.needTarget = false; // 闪不需要目标

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

inline entt::entity CreateAlcoholCard(entt::registry& reg,
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

    return CreateBasicCard(reg, metaInfo, cost, target, pointAndSuit, effect, BasicCardType::ALCOHOL);
}

inline entt::entity CreateFireAttackCard(entt::registry& reg,
                                         const MetaCardInfo& metaInfo,
                                         const CardCost& cost,
                                         const CardPointAndSuit& pointAndSuit,
                                         const CardEffect& effect)
{
    CardTarget target{};
    target.needTarget = true;
    target.minTargets = 1;
    target.maxTargets = 1;
    target.range = 0;

    return CreateStrategyCard(reg, metaInfo, cost, target, pointAndSuit, effect, StrategyCardType::FIRE_ATTACK);
}