/**
 * ************************************************************************
 *
 * @file test_transport.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.1
 * @brief UDP 传输层单元测试
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#include <asio.hpp>
#include <gtest/gtest.h>
#include "src/net/transport/AsioUdpTransport.h"
#include "MockUdpTransport.h"

#include <thread>
#include <chrono>
#include <stdint.h>
#include <array>

using namespace std::chrono_literals;

// ========== Mock Transport 测试 ==========
class MockUdpTransportTest : public ::testing::Test
{
protected:
    MockUdpTransport m_transport;
};

TEST_F(MockUdpTransportTest, CreateTransport)
{
    EXPECT_EQ(m_transport.getSendCount(), 0);
}

TEST_F(MockUdpTransportTest, SendData)
{
    asio::ip::udp::endpoint ep(asio::ip::make_address("127.0.0.1"), 8888);
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};

    m_transport.send(ep, data);

    EXPECT_EQ(m_transport.getSendCount(), 1);
    EXPECT_TRUE(m_transport.hasPacketTo(ep));

    auto packets = m_transport.getPackets();
    ASSERT_EQ(packets.size(), 1);
    EXPECT_EQ(packets[0].to, ep);
    EXPECT_EQ(packets[0].data, data);
}

TEST_F(MockUdpTransportTest, MultipleSends)
{
    asio::ip::udp::endpoint ep1(asio::ip::make_address("127.0.0.1"), 8888);
    asio::ip::udp::endpoint ep2(asio::ip::make_address("127.0.0.1"), 9999);

    std::array<uint8_t, 2> data1 = {1, 2};
    std::array<uint8_t, 3> data2 = {3, 4, 5};
    std::array<uint8_t, 1> data3 = {6};

    m_transport.send(ep1, data1);
    m_transport.send(ep2, data2);
    m_transport.send(ep1, data3);

    EXPECT_EQ(m_transport.getSendCount(), 3);
    EXPECT_TRUE(m_transport.hasPacketTo(ep1));
    EXPECT_TRUE(m_transport.hasPacketTo(ep2));
}

TEST_F(MockUdpTransportTest, ClearPackets)
{
    asio::ip::udp::endpoint ep(asio::ip::make_address("127.0.0.1"), 8888);
    std::array<uint8_t, 3> data = {1, 2, 3};
    m_transport.send(ep, data);

    EXPECT_EQ(m_transport.getSendCount(), 1);

    m_transport.clearPackets();

    EXPECT_EQ(m_transport.getSendCount(), 0);
    EXPECT_FALSE(m_transport.hasPacketTo(ep));
}

TEST_F(MockUdpTransportTest, ThreadSafety)
{
    asio::ip::udp::endpoint ep(asio::ip::make_address("127.0.0.1"), 8888);
    std::vector<uint8_t> data = {0xAA, 0xBB};

    // 并发发送
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++)
    {
        threads.emplace_back(
            [&, i]()
            {
                for (int j = 0; j < 100; j++)
                {
                    m_transport.send(ep, data);
                }
            });
    }

    for (auto& t : threads)
    {
        t.join();
    }

    EXPECT_EQ(m_transport.getSendCount(), 1000);
}

// ========== AsioUdpTransport 测试 ==========
class AsioUdpTransportTest : public ::testing::Test
{
protected:
    void SetUp() override { m_ioc = std::make_unique<asio::io_context>(); }

    void TearDown() override
    {
        if (m_transport)
        {
            m_transport->stop();
        }
        m_transport.reset();
        m_ioc.reset();
    }

    std::unique_ptr<asio::io_context> m_ioc;
    std::unique_ptr<AsioUdpTransport> m_transport;
};

TEST_F(AsioUdpTransportTest, CreateTransport)
{
    EXPECT_NO_THROW(m_transport = std::make_unique<AsioUdpTransport>(m_ioc->get_executor(), 0));
    ASSERT_NE(m_transport, nullptr);
}

TEST_F(AsioUdpTransportTest, BindToPort)
{
    m_transport = std::make_unique<AsioUdpTransport>(m_ioc->get_executor(), 12345);

    // 验证绑定成功（通过不抛出异常）
    SUCCEED();
}

TEST_F(AsioUdpTransportTest, BindToRandomPort)
{
    // 端口 0 表示随机分配
    m_transport = std::make_unique<AsioUdpTransport>(m_ioc->get_executor(), 0);

    SUCCEED();
}

TEST_F(AsioUdpTransportTest, SendData)
{
    m_transport = std::make_unique<AsioUdpTransport>(m_ioc->get_executor(), 0);

    asio::ip::udp::endpoint target(asio::ip::make_address("127.0.0.1"), 8888);
    std::vector<uint8_t> data = {0x11, 0x22, 0x33};

    // 发送数据（实际网络发送）
    EXPECT_NO_THROW(m_transport->send(target, data));
}

TEST_F(AsioUdpTransportTest, StartReceiving)
{
    m_transport = std::make_unique<AsioUdpTransport>(m_ioc->get_executor(), 0);

    bool received = false;
    std::vector<uint8_t> receivedData;

    m_transport->startReceive(
        [&](const asio::ip::udp::endpoint&, std::span<const uint8_t> data)
        {
            received = true;
            receivedData.assign(data.begin(), data.end());
        });

    // 运行短时间（没有实际数据到达）
    m_ioc->run_for(100ms);

    // 没有数据应该到达（因为没有发送方）
    EXPECT_FALSE(received);
}

TEST_F(AsioUdpTransportTest, SendAndReceiveLoopback)
{
    // 创建两个传输实例进行通信
    auto transport1 = std::make_unique<AsioUdpTransport>(m_ioc->get_executor(), 0);
    auto transport2 = std::make_unique<AsioUdpTransport>(m_ioc->get_executor(), 0);

    bool received = false;
    std::vector<uint8_t> receivedData;
    asio::ip::udp::endpoint receivedFrom;

    // transport2 开始接收
    transport2->startReceive(
        [&](const asio::ip::udp::endpoint& from, std::span<const uint8_t> data)
        {
            received = true;
            receivedData.assign(data.begin(), data.end());
            receivedFrom = from;
        });

    // 启动 IO 线程
    std::thread ioThread([this]() { m_ioc->run(); });

    // 稍等片刻确保接收器准备好
    std::this_thread::sleep_for(50ms);

    // transport1 发送数据到 transport2
    std::vector<uint8_t> testData = {0xDE, 0xAD, 0xBE, 0xEF};
    asio::ip::udp::endpoint target(asio::ip::make_address("127.0.0.1"), transport2->localPort());
    transport1->send(target, testData);

    // 等待接收
    std::this_thread::sleep_for(100ms);

    // 停止 IO
    m_ioc->stop();
    ioThread.join();

    // 验证接收到数据
    EXPECT_TRUE(received);
    EXPECT_EQ(receivedData, testData);

    transport1->stop();
    transport2->stop();
}

TEST_F(AsioUdpTransportTest, StopTransport)
{
    m_transport = std::make_unique<AsioUdpTransport>(m_ioc->get_executor(), 0);

    m_transport->startReceive(
        [](const asio::ip::udp::endpoint&, std::span<const uint8_t>)
        {
            // 不应该被调用
            FAIL() << "Receive callback called after stop";
        });

    m_transport->stop();

    // 运行 IO（应该没有任何活动）
    m_ioc->run_for(100ms);

    SUCCEED();
}

TEST_F(AsioUdpTransportTest, MultipleSends)
{
    m_transport = std::make_unique<AsioUdpTransport>(m_ioc->get_executor(), 0);

    asio::ip::udp::endpoint target(asio::ip::make_address("127.0.0.1"), 8888);

    // 连续发送多个数据包
    for (int i = 0; i < 100; i++)
    {
        std::vector<uint8_t> data = {static_cast<uint8_t>(i)};
        EXPECT_NO_THROW(m_transport->send(target, data));
    }
}

// 测试本地端口获取
TEST_F(AsioUdpTransportTest, GetLocalPort)
{
    m_transport = std::make_unique<AsioUdpTransport>(m_ioc->get_executor(), 15000);

    uint16_t port = m_transport->localPort();
    EXPECT_EQ(port, 15000);
}
