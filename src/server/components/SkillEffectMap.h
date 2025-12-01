#pragma once
#include <entt/entt.hpp>
#include <flat_map>
using effect = entt::delegate<void(entt::entity, std::span<entt::entity>, entt::registry&)>;
struct SkillEffectMap
{
    std::flat_map<std::string, effect> effectsMap;
};