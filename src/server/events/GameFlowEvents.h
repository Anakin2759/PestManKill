#pragma once
#include <entt/entt.hpp>
#include <string>
#include <absl/container/inlined_vector.h>
namespace events
{
constexpr size_t MAX_PLAYERS = 8;
struct GameStart
{
    absl::InlinedVector<entt::entity, MAX_PLAYERS> players; // 参与游戏的玩家实体列表
};

struct GameEnd
{
    std::string reason;                                    // 游戏结束原因
    absl::InlinedVector<entt::entity, MAX_PLAYERS> winner; // 获胜玩家实体（若有）
};

struct NextTurn
{
};

struct GotKilled
{
    entt::entity player;        // 死亡的玩家实体
    entt::entity killer;        // 杀死该玩家的实体（若有）
    std::string deathReason;    // 死亡原因描述
};

} // namespace events