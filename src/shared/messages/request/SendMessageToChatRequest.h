/**
 * ************************************************************************
 *
 * @file SendMessageToChatRequest.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.2
 * @brief 发送聊天消息请求
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

struct SendMessageToChatRequest : public MessageBase<SendMessageToChatRequest>
{
    static constexpr uint16_t CMD_ID = CommandID::SEND_MESSAGE_REQ;
    uint32_t sender;
    std::string chatMessage;

    [[nodiscard]] nlohmann::json toJsonImpl() const { return {{"sender", sender}, {"chatMessage", chatMessage}}; }

    static std::expected<SendMessageToChatRequest, MessageError> fromJsonImpl(const nlohmann::json& json)
    {
        try
        {
            SendMessageToChatRequest req;
            req.sender = json.at("sender").get<uint32_t>();
            req.chatMessage = json.at("chatMessage").get<std::string>();
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

    static std::expected<SendMessageToChatRequest, MessageError> deserializeImpl(std::span<const uint8_t>)
    {
        return std::unexpected(MessageError::DeserializeFailed);
    }
};