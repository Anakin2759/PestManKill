/**
 * ************************************************************************
 *
 * @file CreateRoomRequest.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.2
 * @brief 创建房间请求消息
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "../MessageBase.h"
#include "src/shared/common/CommandID.h"
#include <string>
#include <cstring>
#include <algorithm>

struct CreateRoomRequest : public MessageBase<CreateRoomRequest>
{
    static constexpr uint16_t CMD_ID = CommandID::CREATE_ROOM_REQ;
    static constexpr size_t MAX_ROOM_NAME_LEN = 64;

    char roomName[MAX_ROOM_NAME_LEN] = {}; // 房间名（固定长度）
    uint32_t maxPlayers = 5;               // 最大玩家数
    uint8_t gameMode = 0;                  // 游戏模式
    uint8_t reserved[3] = {};              // 对齐4字节

    // 便捷构造函数
    static CreateRoomRequest create(const std::string& name, uint32_t maxPlayers, uint8_t mode = 0)
    {
        CreateRoomRequest req;
        std::strncpy(req.roomName, name.c_str(), MAX_ROOM_NAME_LEN - 1);
        req.maxPlayers = maxPlayers;
        req.gameMode = mode;
        return req;
    }

    // 二进制序列化
    std::expected<std::span<uint8_t>, MessageError> serializeImpl(std::span<uint8_t> buffer) const
    {
        constexpr size_t size = sizeof(CreateRoomRequest);
        if (buffer.size() < size)
        {
            return std::unexpected(MessageError::BufferTooSmall);
        }

        std::memcpy(buffer.data(), this, size);
        return buffer.subspan(0, size);
    }

    static std::expected<CreateRoomRequest, MessageError> deserializeImpl(std::span<const uint8_t> data)
    {
        constexpr size_t size = sizeof(CreateRoomRequest);
        if (data.size() < size)
        {
            return std::unexpected(MessageError::InvalidFormat);
        }

        CreateRoomRequest req;
        std::memcpy(&req, data.data(), size);
        return req;
    }

    // JSON 序列化
    [[nodiscard]] nlohmann::json toJsonImpl() const
    {
        return {{"roomName", std::string(roomName)}, {"maxPlayers", maxPlayers}, {"gameMode", gameMode}};
    }

    static std::expected<CreateRoomRequest, MessageError> fromJsonImpl(const nlohmann::json& j)
    {
        try
        {
            return create(
                j.at("roomName").get<std::string>(), j.at("maxPlayers").get<uint32_t>(), j.value("gameMode", 0));
        }
        catch (...)
        {
            return std::unexpected(MessageError::DeserializeFailed);
        }
    }
};