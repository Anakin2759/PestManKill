/**
 * ************************************************************************
 *
 * @file UseCardMessage.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-01
 * @version 0.1
 * @brief 使用卡牌消息结构体定义
  用于客户端向服务器发送使用卡牌的请求
  包含使用者、卡牌ID和目标列表
  基于JSON格式进行序列化和反序列化
  客户端实现
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <entt/entt.hpp>

struct UseCardMessage
{
    uint32_t player;
    uint32_t card;
    std::vector<uint32_t> targets;

    [[nodiscard]] nlohmann::json toJson() const { return {{"player", player}, {"card", card}, {"targets", targets}}; }

    static UseCardMessage fromJson(const nlohmann::json& json)
    {
        return {.player = json.at("player").get<uint32_t>(),
                .card = json.at("card").get<uint32_t>(),
                .targets = json.at("targets").get<std::vector<uint32_t>>()};
    }
};