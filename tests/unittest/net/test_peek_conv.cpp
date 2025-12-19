/**
 * ************************************************************************
 *
 * @file test_peek_conv.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.1
 * @brief PeekConv 工具函数单元测试
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#include <gtest/gtest.h>
#include "src/net/App/PeekConv.h"
#include <vector>
#include <cstring>

class PeekConvTest : public ::testing::Test
{
protected:
    std::vector<uint8_t> createPacketWithConv(uint32_t conv)
    {
        std::vector<uint8_t> packet(24, 0);
        std::memcpy(packet.data(), &conv, sizeof(conv));
        return packet;
    }
};

// 测试 1: 正常读取 Conv
TEST_F(PeekConvTest, ReadValidConv)
{
    uint32_t testConv = 0x12345678;
    auto packet = createPacketWithConv(testConv);

    uint32_t result = peekConv(packet);
    EXPECT_EQ(result, testConv);
}

// 测试 2: 读取 0 值 Conv
TEST_F(PeekConvTest, ReadZeroConv)
{
    uint32_t testConv = 0;
    auto packet = createPacketWithConv(testConv);

    uint32_t result = peekConv(packet);
    EXPECT_EQ(result, testConv);
}

// 测试 3: 读取最大值 Conv
TEST_F(PeekConvTest, ReadMaxConv)
{
    uint32_t testConv = 0xFFFFFFFF;
    auto packet = createPacketWithConv(testConv);

    uint32_t result = peekConv(packet);
    EXPECT_EQ(result, testConv);
}

// 测试 4: 数据包太短
TEST_F(PeekConvTest, PacketTooShort)
{
    std::vector<uint8_t> shortPacket = {1, 2};

    uint32_t result = peekConv(shortPacket);
    EXPECT_EQ(result, 0); // 应返回 0
}

// 测试 5: 空数据包
TEST_F(PeekConvTest, EmptyPacket)
{
    std::vector<uint8_t> emptyPacket;

    uint32_t result = peekConv(emptyPacket);
    EXPECT_EQ(result, 0);
}

// 测试 6: 恰好 4 字节
TEST_F(PeekConvTest, ExactlyFourBytes)
{
    uint32_t testConv = 0xABCDEF01;
    std::vector<uint8_t> packet(4);
    std::memcpy(packet.data(), &testConv, sizeof(testConv));

    uint32_t result = peekConv(packet);
    EXPECT_EQ(result, testConv);
}

// 测试 7: 多个不同的 Conv 值
TEST_F(PeekConvTest, VariousConvValues)
{
    std::vector<uint32_t> testConvs = {1, 100, 1000, 10000, 0x1234, 0xABCD, 0xDEADBEEF};

    for (uint32_t testConv : testConvs)
    {
        auto packet = createPacketWithConv(testConv);
        uint32_t result = peekConv(packet);
        EXPECT_EQ(result, testConv) << "Failed for conv: " << std::hex << testConv;
    }
}

// 测试 8: 小端序验证
TEST_F(PeekConvTest, LittleEndianVerification)
{
    // 创建小端序的 Conv 值 0x04030201
    std::vector<uint8_t> packet = {0x01, 0x02, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00};

    uint32_t result = peekConv(packet);

    // 在小端序机器上应该是 0x04030201
    // 在大端序机器上会被转换为 0x04030201
    EXPECT_EQ(result, 0x04030201);
}

// 测试 9: span 边界测试
TEST_F(PeekConvTest, SpanBoundary)
{
    std::vector<uint8_t> largePacket(1024, 0);
    uint32_t testConv = 0x99887766;
    std::memcpy(largePacket.data(), &testConv, sizeof(testConv));

    // 只传递前 4 个字节
    std::span<const uint8_t> span(largePacket.data(), 4);
    uint32_t result = peekConv(span);
    EXPECT_EQ(result, testConv);
}

// 测试 10: 连续多个 Conv 提取
TEST_F(PeekConvTest, MultipleConvExtraction)
{
    std::vector<uint32_t> convs = {111, 222, 333};
    std::vector<uint8_t> multiPacket;

    // 构建包含多个 Conv 的数据包
    for (uint32_t conv : convs)
    {
        std::vector<uint8_t> temp(24, 0);
        std::memcpy(temp.data(), &conv, sizeof(conv));
        multiPacket.insert(multiPacket.end(), temp.begin(), temp.end());
    }

    // 逐个提取
    for (size_t i = 0; i < convs.size(); i++)
    {
        std::span<const uint8_t> span(multiPacket.data() + i * 24, 24);
        uint32_t result = peekConv(span);
        EXPECT_EQ(result, convs[i]);
    }
}
