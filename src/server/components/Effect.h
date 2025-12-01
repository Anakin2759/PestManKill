#pragma once
#include <entt/entity/fwd.hpp>
#include <entt/entt.hpp>
#include <vector>
using EffectFunc = entt::delegate<void(entt::entity, std::span<entt::entity>, entt::registry&)>;
using fliterFunc = entt::delegate<std::span<entt::entity>(entt::entity, entt::registry&)>;

struct Effect
{
    entt::entity source;
    std::vector<entt::entity> targets;
    EffectFunc func;
    fliterFunc flit;
    bool isResponse = false;    // 是否为响应效果
    bool canBeResponded = true; // 是否可以被响应（是否允许触发响应效果）
};
