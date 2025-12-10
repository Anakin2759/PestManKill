/**
 * ************************************************************************
 *
 * @file SettlementResponse.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-09
 * @version 0.1
 * @brief 结算响应消息定义
 *
 * 服务器广播给所有客户端的结算响应
 * 包含结算结果和相关信息
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <cstdint>
#include <string>
#include <nlohmann/json.hpp>

struct SettlementResponse
{
    uint32_t player;     // 发起结算的玩家
    uint32_t card;       // 结算的卡牌
    uint32_t target;     // 结算目标
    bool success;        // 结算是否成功
    std::string message; // 结算结果描述

    [[nodiscard]] nlohmann::json toJson() const
    {
        return {{"player", player}, {"card", card}, {"target", target}, {"success", success}, {"message", message}};
    }

    static SettlementResponse fromJson(const nlohmann::json& json)
    {
        return {.player = json.at("player").get<uint32_t>(),
                .card = json.at("card").get<uint32_t>(),
                .target = json.at("target").get<uint32_t>(),
                .success = json.at("success").get<bool>(),
                .message = json.at("message").get<std::string>()};
    }
};
