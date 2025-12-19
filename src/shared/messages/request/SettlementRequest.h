/**
 * ************************************************************************
 *
 * @file SettlementRequest.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.2
 * @brief 结算请求消息
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "../MessageBase.h"
#include "src/shared/common/CommandID.h"
#include <cstdint>

struct SettlementRequest : public MessageBase<SettlementRequest>
{
    static constexpr uint16_t CMD_ID = CommandID::SETTLEMENT_REQ;
    uint32_t player; // 请求结算的玩家
    uint32_t card;   // 要结算的卡牌
    uint32_t target; // 结算目标

    [[nodiscard]] nlohmann::json toJsonImpl() const { return {{"player", player}, {"card", card}, {"target", target}}; }

    static std::expected<SettlementRequest, MessageError> fromJsonImpl(const nlohmann::json& json)
    {
        try
        {
            SettlementRequest req;
            req.player = json.at("player").get<uint32_t>();
            req.card = json.at("card").get<uint32_t>();
            req.target = json.at("target").get<uint32_t>();
            return req;
        }
        catch (...)
        {
            return std::unexpected(MessageError::DeserializeFailed);
        }
    }

    std::expected<std::span<uint8_t>, MessageError> serializeImpl(std::span<uint8_t>) const
    {
        return std::unexpected(MessageError::SerializeFailed);
    }

    static std::expected<SettlementRequest, MessageError> deserializeImpl(std::span<const uint8_t>)
    {
        return std::unexpected(MessageError::DeserializeFailed);
    }
};
