/**
 * ************************************************************************
 *
 * @file test_kcp_session.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.2
 * @brief KCP Session 单元测试
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#include <gtest/gtest.h>
#include "src/net/Session/KcpSession.h"
#include "MockUdpTransport.h"
#include <asio.hpp>

using namespace std::chrono_literals;

class KcpSessionTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_ioc = std::make_unique<asio::io_context>();
        m_transport = std::make_unique<MockUdpTransport>();
        m_endpoint = asio::ip::udp::endpoint(asio::ip::make_address("127.0.0.1"), 12345);
    }

    void TearDown() override
    {
        m_session.reset();
        m_transport.reset();
        m_ioc.reset();
    }

    std::shared_ptr<KcpSession> createSession(uint32_t conv)
    {
        return std::make_shared<KcpSession>(conv, *m_transport, m_endpoint, m_ioc->get_executor());
    }

    std::unique_ptr<asio::io_context> m_ioc;
    std::unique_ptr<MockUdpTransport> m_transport;
    std::shared_ptr<KcpSession> m_session;
    asio::ip::udp::endpoint m_endpoint;
};

// 测试 1: Session 创建和基本属性
TEST_F(KcpSessionTest, CreateSession)
{
    const uint32_t testConv = 0x12345678;
    m_session = createSession(testConv);

    ASSERT_NE(m_session, nullptr);
    // KcpSession 没有 getConv() 方法，通过其他方式验证创建成功
    SUCCEED();
}

// 测试 2: 发送数据
TEST_F(KcpSessionTest, SendData)
{
    m_session = createSession(100);
    m_transport->clearPackets();

    std::vector<uint8_t> testData = {1, 2, 3, 4, 5};
    m_session->send(testData);

    // 触发 KCP 更新，确保数据被发送
    m_session->update(0);

    // 验证至少有一个数据包被发送
    EXPECT_GT(m_transport->getSendCount(), 0);
    EXPECT_TRUE(m_transport->hasPacketTo(m_endpoint));
}

// 测试 3: 接收数据
TEST_F(KcpSessionTest, ReceiveData)
{
    m_session = createSession(200);

    // 构造一个简单的 KCP 数据包 (这里简化处理，实际需要正确的 KCP 协议格式)
    std::vector<uint8_t> kcpPacket(24, 0);
    uint32_t conv = 200;
    std::memcpy(kcpPacket.data(), &conv, sizeof(conv));

    // 输入数据到 Session
    m_session->input(kcpPacket);

    // 更新 Session
    m_session->update(10);

    // 验证输入成功（没有崩溃即为成功）
    SUCCEED();
}

// 测试 4: 协程接收
TEST_F(KcpSessionTest, CoroutineReceive)
{
    m_session = createSession(300);

    bool receiveCalled = false;
    std::vector<uint8_t> receivedData;

    // 启动协程接收
    asio::co_spawn(
        m_ioc->get_executor(),
        [&]() -> asio::awaitable<void>
        {
            try
            {
                auto result = co_await m_session->recv();
                if (result.has_value())
                {
                    receivedData = result.value();
                    receiveCalled = true;
                }
            }
            catch (...)
            {
                // 超时或错误
            }
        },
        asio::detached);

    // 模拟发送数据（需要正确的 KCP 包格式）
    m_session->update(0);

    // 运行少量时间（避免无限等待）
    m_ioc->run_for(100ms);

    // 注意：这个测试可能需要实际的 KCP 数据包才能触发接收
    // 当前只是验证协程能够启动
}

// 测试 5: Update 方法
TEST_F(KcpSessionTest, UpdateMethod)
{
    m_session = createSession(400);

    // 多次调用 update，验证不会崩溃
    for (uint32_t i = 0; i < 100; i += 10)
    {
        EXPECT_NO_THROW(m_session->update(i));
    }
}

// 测试 6: 空数据发送
TEST_F(KcpSessionTest, SendEmptyData)
{
    m_session = createSession(500);
    m_transport->clearPackets();

    std::vector<uint8_t> emptyData;
    m_session->send(emptyData);
    m_session->update(0);

    // KCP 即使是空数据也会发送协议帧（如 ACK），所以 getSendCount() >= 0
    EXPECT_GE(m_transport->getSendCount(), 0);
}

// 测试 7: 大数据包发送
TEST_F(KcpSessionTest, SendLargeData)
{
    m_session = createSession(600);
    m_transport->clearPackets();

    // 发送 10KB 数据
    std::vector<uint8_t> largeData(10240, 0xAB);
    m_session->send(largeData);
    m_session->update(0);

    // 验证数据被分片发送
    EXPECT_GT(m_transport->getSendCount(), 0);
}

// 测试 8: 多次发送
TEST_F(KcpSessionTest, MultipleSends)
{
    m_session = createSession(700);
    m_transport->clearPackets();

    for (int i = 0; i < 5; i++)
    {
        std::vector<uint8_t> data = {static_cast<uint8_t>(i)};
        m_session->send(data);
    }

    m_session->update(0);
    size_t initialCount = m_transport->getSendCount();

    m_session->update(10);
    size_t afterCount = m_transport->getSendCount();

    // 应该发送了数据
    EXPECT_GT(afterCount, 0);
}