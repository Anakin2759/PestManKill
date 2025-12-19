/**
 * ************************************************************************
 *
 * @file ConnectedRequest.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.1
 * @brief 客户端连接请求消息
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "../MessageBase.h"
#include "src/shared/common/CommandID.h"
#include <cstring>
#include <string>

struct ConnectedRequest : public MessageBase<ConnectedRequest>
{
    static constexpr uint16_t CMD_ID = CommandID::CONNECTED;
    static constexpr size_t MAX_PLAYER_NAME_LEN = 32;

    char playerName[MAX_PLAYER_NAME_LEN] = {}; // 玩家名称
    uint32_t clientVersion = 1;                // 客户端版本号

    static ConnectedRequest create(const std::string& name, uint32_t version = 1)
    {
        ConnectedRequest req;
        std::strncpy(req.playerName, name.c_str(), MAX_PLAYER_NAME_LEN - 1);
        req.clientVersion = version;
        return req;
    }

    std::expected<std::span<uint8_t>, MessageError> serializeImpl(std::span<uint8_t> buffer) const
    {
        constexpr size_t size = sizeof(ConnectedRequest);
        if (buffer.size() < size)
        {
            return std::unexpected(MessageError::BufferTooSmall);
        }

        std::memcpy(buffer.data(), this, size);
        return buffer.subspan(0, size);
    }

    static std::expected<ConnectedRequest, MessageError> deserializeImpl(std::span<const uint8_t> data)
    {
        constexpr size_t size = sizeof(ConnectedRequest);
        if (data.size() < size)
        {
            return std::unexpected(MessageError::InvalidFormat);
        }

        ConnectedRequest req;
        std::memcpy(&req, data.data(), size);
        return req;
    }

    [[nodiscard]] nlohmann::json toJsonImpl() const
    {
        return {{"playerName", std::string(playerName)}, {"clientVersion", clientVersion}};
    }

    static std::expected<ConnectedRequest, MessageError> fromJsonImpl(const nlohmann::json& j)
    {
        try
        {
            return create(j.at("playerName").get<std::string>(), j.value("clientVersion", 1));
        }
        catch (...)
        {
            return std::unexpected(MessageError::DeserializeFailed);
        }
    }
};
