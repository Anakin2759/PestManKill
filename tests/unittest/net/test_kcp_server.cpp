// tests/net/test_kcp_server.cpp
#include <gtest/gtest.h>
#include "src/net/kcp/KcpServer.h"
#include "MockUdpTransport.h"

using namespace std::chrono_literals;

TEST(KcpServerTest, CreatesSessionOnNewConv)
{
    MockUdpTransport transport;

    // 创建 server
    KcpServer server(transport);

    // 模拟两个不同 client endpoint
    asio::ip::udp::endpoint ep1(asio::ip::make_address("127.0.0.1"), 10001);
    asio::ip::udp::endpoint ep2(asio::ip::make_address("127.0.0.1"), 10002);

    // 构造假的 KCP 包头（conv=101, 102）
    std::vector<uint8_t> packet1(24, 0);
    std::vector<uint8_t> packet2(24, 0);
    uint32_t conv1 = 101;
    uint32_t conv2 = 102;
    std::memcpy(packet1.data(), &conv1, sizeof(conv1));
    std::memcpy(packet2.data(), &conv2, sizeof(conv2));

    // 将假包交给 server input
    server.input(ep1, packet1);
    server.input(ep2, packet2);

    // 调用 update 模拟定时器驱动
    server.update(10);

    // 检查 session 数量
    EXPECT_EQ(server.sessionCount(), 2);

    // 检查 conv 是否正确映射
    auto s1 = server.getSession(conv1);
    auto s2 = server.getSession(conv2);
    ASSERT_NE(s1, nullptr);
    ASSERT_NE(s2, nullptr);
    EXPECT_EQ(s1->getConv(), conv1);
    EXPECT_EQ(s2->getConv(), conv2);
}

TEST(KcpServerTest, SessionIsolation)
{
    MockUdpTransport transport;
    KcpServer server(transport);

    asio::ip::udp::endpoint ep1(asio::ip::make_address("127.0.0.1"), 10001);
    asio::ip::udp::endpoint ep2(asio::ip::make_address("127.0.0.1"), 10002);

    std::vector<uint8_t> packet1(24, 0);
    std::vector<uint8_t> packet2(24, 0);
    uint32_t conv1 = 201;
    uint32_t conv2 = 202;
    std::memcpy(packet1.data(), &conv1, sizeof(conv1));
    std::memcpy(packet2.data(), &conv2, sizeof(conv2));

    server.input(ep1, packet1);
    server.input(ep2, packet2);

    server.update(10);

    // 给 s1 发数据
    auto s1 = server.getSession(conv1);
    auto s2 = server.getSession(conv2);

    std::vector<uint8_t> payload = {1, 2, 3};
    s1->send(payload);
    s1->update(20);
    s2->update(20); // s2 不会收到 s1 的包

    // 检查 transport 只发给 ep1
    ASSERT_FALSE(transport.packets.empty());
    for (auto& pkt : transport.packets)
    {
        EXPECT_EQ(pkt.to, ep1);
    }
}
