/**
 * ************************************************************************
 *
 * @file framecodec.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-17
 * @version 0.1
 * @brief 帧数据包编解码器
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once
#include <cstdint>
#include <span>
#include <expected>
#include <bit>
#include <algorithm>
#include "FrameHeader.h"

enum class CodecError
{
    BufferTooSmall,   // 缓冲区过小
    InvalidMagic,     // 无效的魔数
    IncompletePayload // 不完整的载荷
};

/**
 * @brief 编码帧数据包
 * @return 成功返回编码后的子切片，失败返回错误码
 */
inline std::expected<std::span<uint8_t>, CodecError>
    encodeFrame(std::span<uint8_t> buffer, uint16_t cmd, std::span<const uint8_t> payload)
{
    const size_t total_size = sizeof(FrameHeader) + payload.size();

    if (buffer.size() < total_size) [[unlikely]]
    {
        return std::unexpected(CodecError::BufferTooSmall);
    }

    // 优化：直接在目标缓冲区构造 Header，避免中间变量拷贝
    auto* header = reinterpret_cast<FrameHeader*>(buffer.data());
    *header = FrameHeader{.cmd = cmd, .length = static_cast<uint16_t>(payload.size())};

    // 拷贝 payload
    if (!payload.empty())
    {
        std::copy(payload.begin(), payload.end(), buffer.begin() + sizeof(FrameHeader));
    }

    return buffer.subspan(0, total_size);
}

/**
 * @brief 解码帧数据包
 * @return 成功返回解出的 Payload span
 */
struct DecodeResult
{
    uint16_t cmd;
    std::span<const uint8_t> payload;
};

inline std::expected<DecodeResult, CodecError> decodeFrame(std::span<const uint8_t> buffer)
{
    if (buffer.size() < sizeof(FrameHeader)) [[unlikely]]
    {
        return std::unexpected(CodecError::BufferTooSmall);
    }

    // 使用 bit_cast 或指针映射，避免 memcpy 整个结构体
    const auto& header = *reinterpret_cast<const FrameHeader*>(buffer.data());

    if (header.magic != 0x55AA) [[unlikely]]
    {
        return std::unexpected(CodecError::InvalidMagic);
    }

    if (buffer.size() < sizeof(FrameHeader) + header.length) [[unlikely]]
    {
        return std::unexpected(CodecError::IncompletePayload);
    }

    return DecodeResult{.cmd = header.cmd, .payload = buffer.subspan(sizeof(FrameHeader), header.length)};
}