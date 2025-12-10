/**
 * ************************************************************************
 *
 * @file TargetSelectionEvents.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-09
 * @version 0.1
 * @brief 目标选择相关事件定义
 *
 * 定义卡牌使用流程中的目标选择事件
 * - 开始选择目标
 * - 目标选择完成
 * - 取消目标选择
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include <vector>
#include <cstdint>

namespace events
{

/**
 * @brief 开始选择目标事件
 * 当玩家点击使用卡牌按钮时触发
 */
struct StartTargetSelection
{
    entt::entity cardEntity; // 要使用的卡牌实体
    uint32_t minTargets;     // 最小目标数
    uint32_t maxTargets;     // 最大目标数
    uint32_t range;          // 选择范围（0表示无限制）
};

/**
 * @brief 目标被选中事件
 * 当玩家点击某个可选目标时触发
 */
struct TargetSelected
{
    entt::entity targetEntity; // 被选中的目标实体
};

/**
 * @brief 目标被取消选择事件
 * 当玩家再次点击已选中的目标时触发
 */
struct TargetDeselected
{
    entt::entity targetEntity; // 被取消选择的目标实体
};

/**
 * @brief 确认目标选择事件
 * 当玩家确认选择的目标并准备发送使用卡牌请求时触发
 */
struct ConfirmTargetSelection
{
    entt::entity cardEntity;           // 要使用的卡牌实体
    std::vector<entt::entity> targets; // 选择的目标实体列表
};

/**
 * @brief 取消目标选择事件
 * 当玩家取消目标选择时触发
 */
struct CancelTargetSelection
{
    entt::entity cardEntity; // 取消使用的卡牌实体
};

/**
 * @brief 使用卡牌请求发送事件
 * 当客户端向服务器发送使用卡牌请求时触发
 */
struct UseCardRequestSent
{
    uint32_t cardId;                 // 卡牌ID
    std::vector<uint32_t> targetIds; // 目标ID列表
};

} // namespace events
