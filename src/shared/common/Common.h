#pragma once
#include <cstdint>
#include <entt/entt.hpp>
#include <span>
// 回合的阶段
enum class TurnPhase : uint8_t
{
    GAME_START,
    START,   // 回合开始阶段
    JUDGE,   // 判定阶段
    DRAW,    // 摸牌阶段
    PLAY,    // 出牌阶段
    DISCARD, // 弃牌阶段
    END,     // 回合结束阶段
    GAME_OVER
};

enum class TriggerMoment : uint8_t
{
    BEFORE,
    AFTER,
    DURING
};

enum class GameMode : uint8_t
{
    CLASSIC_FIVE,
    CLASSIC_EIGHT,
    TEAM_BATTLE,
    CHANLLENGE_PEST
};

enum class GenderType : uint8_t
{
    FEMALE,
    MALE,
    OTHER
};

enum class IdentityType : uint8_t
{
    MEMBER,
    OWNER,
    ADMIN,
    ROGUE,
    PESTMAN
};
enum class FactionType : uint8_t
{
    VIRGIN,
    PLAYMAN,
    BOOMER
};

enum class SkillType : uint8_t
{
    PASSIVE,  // 被动技能
    ACTIVE,   // 主动技能
    REACTIVE, // 反应技能
    LOCKED,
    PHASELIMITED,
    AWAKENING // 觉醒
};
/************************* */

enum class CardType : uint8_t
{
    BASIC,
    STRATEGY,
    EQUIP
};

enum class BasicCardType : uint8_t
{
    STRIKE,  // 攻击
    DODGE,   // 闪避
    ALCOHOL, // 酒
    PEACH,   // 桃子
};

enum class StrategyCardType : uint8_t
{
    FIRE_ATTACK,           // 火攻
    DUEL,                  // 决斗
    SNATCH,                // 顺手牵羊
    DISMANTLE,             // 过河拆桥
    LIGHTNING,             // 闪电
    SOMETHING_FOR_NOTHING, // 无中生有
    WARD                   // 无懈可击

};

enum class EquipCardType : uint8_t
{
    WEAPON,       // 武器
    ARMOR,        // 防具
    ATTACK_HORSE, // 攻击马
    DEFENSE_HORSE // 防御马
};

enum class StatusType : uint8_t
{
    FLIP,   // 翻面
    TAP,    // 横置
    DRUNKED // 醉酒
};

enum class SuitType : uint8_t
{
    HEARTS,   // 红桃
    DIAMONDS, // 方块
    CLUBS,    // 梅花
    SPADES,   // 黑桃
    JOKER     // 小王或大王
};

using Effect = entt::delegate<void(entt::entity, std::span<entt::entity>, entt::registry&)>;