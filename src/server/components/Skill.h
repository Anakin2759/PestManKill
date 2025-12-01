/**
 * ************************************************************************
 *
 * @file Skill.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-11-18
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
#include <entt/entity/fwd.hpp>
#include <string>
#include <functional>
#include <entt/entt.hpp>
#include <array>
#include "src/shared/common/Common.h"

struct MetaSkillInfo
{
    std::string name;
    std::string description; // 新增描述字段
};

struct SkillTarget
{
    bool needTarget = true;
    uint8_t maxTargets = 1;
    uint8_t minTargets = 1;

    std::function<bool(entt::entity)> filter; // 目标筛选规则
};

inline entt::entity CreateSkill(const MetaSkillInfo& info, const Effect& effect, entt::registry& reg)
{
    auto ent = reg.create();
    reg.emplace<MetaSkillInfo>(ent, info);
    reg.emplace<Effect>(ent, effect);
    return ent;
}

inline void WuShengFunc(entt::entity user, std::span<entt::entity> targets, entt::registry& reg)
{
    // 武圣：红牌当作【杀】使用或打出。
    // 这里作为占位实现：把技能调用映射为对目标逐一触发普通的 Slash 效果。
    // 真实实现应在触发时检查玩家选择的牌（花色为 HEARTS/DIAMONDS），
    // 并把该牌视作攻击牌来消耗并结算效果；此处仅演示如何调用 SlashEffect。

    if (targets.empty())
    {
        return;
    }

    // 对每个目标构造单元素 span 并调用 SlashEffect
    for (auto target : targets)
    {
        std::array<entt::entity, 1> tmp{target};
        std::span<entt::entity> single{tmp.data(), 1};
    }
};
inline void WuShuangFunc(entt::entity user, std::span<entt::entity> target, entt::registry& reg) {

};