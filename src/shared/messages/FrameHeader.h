/**
 * ************************************************************************
 *
 * @file PacketHeader.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-12
 * @version 0.1
 * @brief 网络数据包头定义
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct FrameHeader
{
    uint16_t header = 0x55AA; // 帧头
    uint16_t cmd;             // 指令 ID
    uint16_t length;          // 载荷长度
};
#pragma pack(pop)