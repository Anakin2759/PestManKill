/**
 * ************************************************************************
 *
 * @file DiscardCardMessage.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-01
 * @version 0.1
 * @brief 弃牌消息结构体定义
  用于客户端向服务器发送弃牌的请求
  包含弃牌者和弃牌列表
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

struct DiscardCardMessage
{
    uint32_t player;
    std::vector<uint32_t> cards;

    [[nodiscard]] nlohmann::json toJson() const { return {{"player", player}, {"cards", cards}}; }

    static DiscardCardMessage fromJson(const nlohmann::json& json)
    {
        return {.player = json.at("player").get<uint32_t>(), .cards = json.at("cards").get<std::vector<uint32_t>>()};
    }
};