/**
 * ************************************************************************
 *
 * @file UseCardRequest.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.2
 * @brief 使用卡牌请求消息
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
#include <vector>

struct UseCardRequest : public MessageBase<UseCardRequest>
{
    static constexpr uint16_t CMD_ID = CommandID::USE_CARD_REQ;
    uint32_t player;
    uint32_t card;
    std::vector<uint32_t> targets;

    [[nodiscard]] nlohmann::json toJsonImpl() const
    {
        return {{"player", player}, {"card", card}, {"targets", targets}};
    }

    static std::expected<UseCardRequest, MessageError> fromJsonImpl(const nlohmann::json& json)
    {
        try
        {
            UseCardRequest req;
            req.player = json.at("player").get<uint32_t>();
            req.card = json.at("card").get<uint32_t>();
            req.targets = json.at("targets").get<std::vector<uint32_t>>();
            return req;
        }
        catch (...)
        {
            return std::unexpected(MessageError::DeserializeFailed);
        }
    }

    // 二进制序列化暂不实现（根据需要添加）
    std::expected<std::span<uint8_t>, MessageError> serializeImpl(std::span<uint8_t>) const
    {
        return std::unexpected(MessageError::SerializeFailed);
    }

    static std::expected<UseCardRequest, MessageError> deserializeImpl(std::span<const uint8_t>)
    {
        return std::unexpected(MessageError::DeserializeFailed);
    }
};