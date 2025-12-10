/**
 * ************************************************************************
 *
 * @file UseCardResponse.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-09
 * @version 0.1
 * @brief 使用卡牌响应消息定义
 *
 * 服务器广播给所有客户端的使用卡牌响应
 * 包含使用者、卡牌、目标等信息
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <cstdint>
#include <vector>
#include <nlohmann/json.hpp>

struct UseCardResponse
{
    uint32_t player;               // 使用卡牌的玩家
    uint32_t card;                 // 使用的卡牌
    std::vector<uint32_t> targets; // 目标列表
    bool success;                  // 是否成功使用
    std::string message;           // 附加消息

    [[nodiscard]] nlohmann::json toJson() const
    {
        return {{"player", player}, {"card", card}, {"targets", targets}, {"success", success}, {"message", message}};
    }

    static UseCardResponse fromJson(const nlohmann::json& json)
    {
        return {.player = json.at("player").get<uint32_t>(),
                .card = json.at("card").get<uint32_t>(),
                .targets = json.at("targets").get<std::vector<uint32_t>>(),
                .success = json.at("success").get<bool>(),
                .message = json.at("message").get<std::string>()};
    }
};
