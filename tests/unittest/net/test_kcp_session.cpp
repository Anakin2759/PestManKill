/**
 * ************************************************************************
 *
 * @file test_kcp_session.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-17
 * @version 0.1
 * @brief 测试 KCP 会话基本功能
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#include <gtest/gtest.h>
#include "src/net/kcp/KcpSession.h"
#include "MockUdpTransport.h"

class KcpServerTest : public ::testing::Test
{
protected:
    MockUdpTransport mockTransport;
    TestServer server{mockTransport};
    asio::ip::udp::endpoint clientAddr = asio::ip::udp::decode_address("127.0.0.1"), 12345;
};

// 1. 测试收到合法的 KCP 包时是否自动创建 Session
TEST_F(KcpServerTest, CreateSessionOnInput)
{
    uint32_t testConv = 0x12345678;
    // 构造一个简单的 KCP 头部 (前4字节是 conv，小端序)
    uint8_t rawPacket[24] = {0};
    std::memcpy(rawPacket, &testConv, sizeof(uint32_t));

    // 输入数据
    server.input(clientAddr, rawPacket);

    // 验证
    EXPECT_TRUE(server.events[testConv].created);
    EXPECT_FALSE(server.events[testConv].closed);
}

// 2. 测试相同 Conv 的包是否分发到同一个 Session
TEST_F(KcpServerTest, DispatchToExistingSession)
{
    uint32_t testConv = 1001;
    uint8_t rawPacket[24] = {0};
    std::memcpy(rawPacket, &testConv, sizeof(uint32_t));

    server.input(clientAddr, rawPacket);
    server.input(clientAddr, rawPacket);

    // 虽然调用了两次 input，但 onSession 应该只被触发一次
    // (因为 TestServer 里的 map 会覆盖记录，我们看 count 也可以)
    EXPECT_EQ(server.events.size(), 1);
}

// 3. 测试超时清理功能
TEST_F(KcpServerTest, SessionTimeout)
{
    uint32_t testConv = 2002;
    uint8_t rawPacket[24] = {0};
    std::memcpy(rawPacket, &testConv, sizeof(uint32_t));

    server.input(clientAddr, rawPacket);
    ASSERT_TRUE(server.events[testConv].created);

    // 模拟时间流逝：调用 update，设置超时阈值为 0 秒
    server.update(1000, std::chrono::seconds(0));

    EXPECT_TRUE(server.events[testConv].closed);
}