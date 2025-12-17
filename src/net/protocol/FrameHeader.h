
#pragma once
// 增加 1 字节对齐，防止编译器插入填充字节导致帧格式与预期不符
#pragma pack(push, 1)
struct FrameHeader
{
    uint16_t magic = 0x55AA; // 语义更明确的命名
    uint16_t cmd = 0;        // 指令 ID
    uint16_t length = 0;     // 载荷长度
};
#pragma pack(pop)