/**
 * ************************************************************************
 *
 * @file SendMessageToChatResponse.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.2
 * @brief 发送聊天消息响应
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
#include <string>

struct SendMessageToChatResponse : public MessageBase<SendMessageToChatResponse>
{
    static constexpr uint16_t CMD_ID = CommandID::SEND_MESSAGE_RESP;

    uint32_t sender;
    std::string chatMessage;

    [[nodiscard]] nlohmann::json toJsonImpl() const { return {{"sender", sender}, {"chatMessage", chatMessage}}; }

    static std::expected<SendMessageToChatResponse, MessageError> fromJsonImpl(const nlohmann::json& json)
    {
        try
        {
            SendMessageToChatResponse resp;
            resp.sender = json.at("sender").get<uint32_t>();
            resp.chatMessage = json.at("chatMessage").get<std::string>();
            return resp;
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

    static std::expected<SendMessageToChatResponse, MessageError> deserializeImpl(std::span<const uint8_t>)
    {
        return std::unexpected(MessageError::DeserializeFailed);
    }
};