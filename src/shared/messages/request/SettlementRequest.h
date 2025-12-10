/**
 * ************************************************************************
 *
 * @file SettlementRequest.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-09
 * @version 0.1
 * @brief 结算请求消息定义
 *
 * 客户端向服务器发送的结算请求
 * 用于请求对卡牌效果进行结算
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>

struct SettlementRequest
{
    uint32_t player; // 请求结算的玩家
    uint32_t card;   // 要结算的卡牌
    uint32_t target; // 结算目标

    [[nodiscard]] nlohmann::json toJson() const { return {{"player", player}, {"card", card}, {"target", target}}; }

    static SettlementRequest fromJson(const nlohmann::json& json)
    {
        return {.player = json.at("player").get<uint32_t>(),
                .card = json.at("card").get<uint32_t>(),
                .target = json.at("target").get<uint32_t>()};
    }
};
