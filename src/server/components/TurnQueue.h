/**
 * ************************************************************************
 *
 * @file TurnQueue.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-11-19
 * @version 0.1
 * @brief 回合队列组件，管理角色的行动顺序,是一个循环队列
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once
#include "src/shared/utils/CircularQueue.h"
#include <entt/entt.hpp>
struct TurnQueue
{
    // NOLINTNEXTLINE
    CircularQueue<entt::entity, 8> queue; // 最多支持8个角色的循环队列
};