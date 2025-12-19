/**
 * ************************************************************************
 *
 * @file CreateRoomResponse.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.2
 * @brief 创建房间响应消息
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

struct CreateRoomResponse : public MessageBase<CreateRoomResponse>
{
    static constexpr uint16_t CMD_ID = CommandID::CREATE_ROOM_RESP;

    uint32_t roomId = 0;      // 房间ID（0表示创建失败）
    uint8_t success = 0;      // 是否成功（0=失败，1=成功）
    uint8_t errorCode = 0;    // 错误码（成功时为0）
    uint8_t reserved[2] = {}; // 对齐4字节

    static CreateRoomResponse createSuccess(uint32_t roomId)
    {
        CreateRoomResponse resp;
        resp.roomId = roomId;
        resp.success = 1;
        resp.errorCode = 0;
        return resp;
    }

    static CreateRoomResponse createFailed(uint8_t errorCode)
    {
        CreateRoomResponse resp;
        resp.roomId = 0;
        resp.success = 0;
        resp.errorCode = errorCode;
        return resp;
    }

    std::expected<std::span<uint8_t>, MessageError> serializeImpl(std::span<uint8_t> buffer) const
    {
        constexpr size_t size = sizeof(CreateRoomResponse);
        if (buffer.size() < size)
        {
            return std::unexpected(MessageError::BufferTooSmall);
        }

        std::memcpy(buffer.data(), this, size);
        return buffer.subspan(0, size);
    }

    static std::expected<CreateRoomResponse, MessageError> deserializeImpl(std::span<const uint8_t> data)
    {
        constexpr size_t size = sizeof(CreateRoomResponse);
        if (data.size() < size)
        {
            return std::unexpected(MessageError::InvalidFormat);
        }

        CreateRoomResponse resp;
        std::memcpy(&resp, data.data(), size);
        return resp;
    }

    [[nodiscard]] nlohmann::json toJsonImpl() const
    {
        return {{"roomId", roomId}, {"success", success != 0}, {"errorCode", errorCode}};
    }

    static std::expected<CreateRoomResponse, MessageError> fromJsonImpl(const nlohmann::json& j)
    {
        try
        {
            CreateRoomResponse resp;
            resp.roomId = j.at("roomId").get<uint32_t>();
            resp.success = j.at("success").get<bool>() ? 1 : 0;
            resp.errorCode = j.value("errorCode", 0);
            return resp;
        }
        catch (...)
        {
            return std::unexpected(MessageError::DeserializeFailed);
        }
    }
};