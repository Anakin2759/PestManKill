#pragma once
#include <cstdint>

struct PacketHeader
{
    uint32_t seq;  // 序列号
    uint32_t ack;  // 确认序列号（用于可靠消息）
    uint16_t type; // 用短整数区分消息类型
    uint16_t size; // payload 字节大小
};