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
    MEMBER, //群友
    OWNER, // 版主
    ADMIN, // 管理员
    ROGUE, // 狂徒
    PESTMAN // 害虫
};
enum class FactionType : uint8_t
{
    VIRGIN, // 处男
    PLAYMAN, // 玩咖
    BOOMER //老登
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

// 消息类型枚举
// NOLINTNEXTLINE
enum class MessageType : uint16_t
{
    HEARTBEAT = 0x0001, // 心跳包
    LOGIN,              // 登录请求
    LOGOUT,             // 登出请求
    USE_CARD = 0x0100,  // 使用卡牌
    DRAW_CARD,          // 抽卡
    DISCARD_CARD,       // 弃牌
    END_TURN,           // 结束回合
    NEXT_PHASE,         // 下一个阶段
    DAMAGE,             // 伤害

    CHOOSING_TARGET, // 选择目标

    CHAT_MESSAGE = 0x0200,  // 聊天消息
    GAME_STATE = 0x0300,    // 游戏状态同步
    ERROR_MESSAGE = 0x0F00, // 错误消息
    ACK = 0xFFFF            // 确认包
};

enum class TargetType : uint8_t
{
    EQUIPMENT,
    HANDCARD,
    CHARACTER
};