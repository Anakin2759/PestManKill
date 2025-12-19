/**
 * ************************************************************************
 *
 * @file test_kcp_server.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.2
 * @brief KCP Server 和 Client 单元测试
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#include <gtest/gtest.h>
#include "src/net/App/Server.h"
#include "src/net/App/Client.h"
#include "MockUdpTransport.h"
#include <asio.hpp>

using namespace std::chrono_literals;

// ========== Server 测试助手类 ==========
class TestServer : public Server
{
public:
    explicit TestServer(IUdpTransport& transport, asio::any_io_executor exec) : Server(transport, std::move(exec), 2) {}

    struct SessionEvent
    {
        uint32_t conv = 0;
        bool created = false;
        bool closed = false;
    };
    std::unordered_map<uint32_t, SessionEvent> events;

    [[nodiscard]] size_t sessionCount() const { return m_sessions.size(); }

    [[nodiscard]] std::shared_ptr<KcpSession> getSession(uint32_t conv)
    {
        auto it = m_sessions.find(conv);
        return it != m_sessions.end() ? it->second : nullptr;
    }

    // 公开访问 protected 方法用于测试
    void testInput(const asio::ip::udp::endpoint& from, std::span<const uint8_t> data) { input(from, data); }

    void testUpdate(uint32_t now_ms, std::chrono::seconds timeout = std::chrono::seconds(30))
    {
        update(now_ms, timeout);
    }

protected:
    void onSession(uint32_t conv, std::shared_ptr<KcpSession> sess) override
    {
        // 不调用 Server::onSession(conv, sess) 避免启动会阻塞的 playerRoutine 协程
        // 直接调用 KcpEndpoint 的版本来注册 session
        KcpEndpoint::onSession(conv, sess);
        events[conv].conv = conv;
        events[conv].created = true;
    }

    void onSessionClosed(uint32_t conv) override
    {
        Server::onSessionClosed(conv);
        events[conv].closed = true;
    }
};

// ========== Server 测试 Fixture ==========
class KcpServerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_ioc = std::make_unique<asio::io_context>();
        m_transport = std::make_unique<MockUdpTransport>();
        m_server = std::make_unique<TestServer>(*m_transport, m_ioc->get_executor());
    }

    void TearDown() override
    {
        if (m_server)
        {
            m_server->stop();
        }
        m_server.reset();
        m_transport.reset();
        m_ioc.reset();
    }

    std::vector<uint8_t> createKcpPacket(uint32_t conv)
    {
        std::vector<uint8_t> packet(24, 0);
        std::memcpy(packet.data(), &conv, sizeof(conv));
        return packet;
    }

    std::unique_ptr<asio::io_context> m_ioc;
    std::unique_ptr<MockUdpTransport> m_transport;
    std::unique_ptr<TestServer> m_server;
};

// 测试 1: 创建 Server
TEST_F(KcpServerTest, CreateServer)
{
    ASSERT_NE(m_server, nullptr);
    EXPECT_EQ(m_server->sessionCount(), 0);
}

// 测试 2: 接收新连接时创建 Session
TEST_F(KcpServerTest, CreateSessionOnNewConv)
{
    asio::ip::udp::endpoint ep1(asio::ip::make_address("127.0.0.1"), 10001);
    asio::ip::udp::endpoint ep2(asio::ip::make_address("127.0.0.1"), 10002);

    auto packet1 = createKcpPacket(101);
    auto packet2 = createKcpPacket(102);

    m_server->testInput(ep1, packet1);
    m_server->testInput(ep2, packet2);

    // 运行 io_context 处理异步协程
    m_ioc->poll();

    m_server->testUpdate(10);

    EXPECT_EQ(m_server->sessionCount(), 2);

    auto s1 = m_server->getSession(101);
    auto s2 = m_server->getSession(102);
    ASSERT_NE(s1, nullptr);
    ASSERT_NE(s2, nullptr);
    // KcpSession 没有 getConv() 方法，通过非空指针验证创建成功
    EXPECT_TRUE(m_server->events[101].created);
    EXPECT_TRUE(m_server->events[102].created);
}

// 测试 3: 相同 Conv 复用 Session
TEST_F(KcpServerTest, ReuseSameConvSession)
{
    asio::ip::udp::endpoint ep(asio::ip::make_address("127.0.0.1"), 10001);
    auto packet = createKcpPacket(201);

    m_server->testInput(ep, packet);
    m_server->testInput(ep, packet); // 再次输入相同 conv

    // 运行 io_context 处理异步协程
    m_ioc->poll();

    m_server->testUpdate(10);

    EXPECT_EQ(m_server->sessionCount(), 1);
    EXPECT_EQ(m_server->events.size(), 1); // 只创建了一次
}

// 测试 4: Session 超时清理
TEST_F(KcpServerTest, SessionTimeout)
{
    asio::ip::udp::endpoint ep(asio::ip::make_address("127.0.0.1"), 10001);
    auto packet = createKcpPacket(301);

    m_server->testInput(ep, packet);
    m_ioc->poll();
    m_server->testUpdate(10);

    ASSERT_EQ(m_server->sessionCount(), 1);
    ASSERT_TRUE(m_server->events[301].created);

    // 模拟超时：设置超时时间为 0 秒
    m_server->testUpdate(1000, std::chrono::seconds(0));

    EXPECT_EQ(m_server->sessionCount(), 0);
    EXPECT_TRUE(m_server->events[301].closed);
}

// 测试 5: 多个 Session 独立性
TEST_F(KcpServerTest, SessionIsolation)
{
    asio::ip::udp::endpoint ep1(asio::ip::make_address("127.0.0.1"), 10001);
    asio::ip::udp::endpoint ep2(asio::ip::make_address("127.0.0.1"), 10002);

    m_server->testInput(ep1, createKcpPacket(401));
    m_server->testInput(ep2, createKcpPacket(402));
    m_ioc->poll();
    m_server->testUpdate(10);

    auto s1 = m_server->getSession(401);
    auto s2 = m_server->getSession(402);

    ASSERT_NE(s1, nullptr);
    ASSERT_NE(s2, nullptr);
    EXPECT_NE(s1, s2); // 不是同一个对象

    // 给 s1 发送数据
    m_transport->clearPackets();
    std::vector<uint8_t> testData = {1, 2, 3};
    s1->send(testData);
    s1->update(20);

    // 验证数据只发给 ep1
    EXPECT_TRUE(m_transport->hasPacketTo(ep1));
    auto packets = m_transport->getPackets();
    for (const auto& pkt : packets)
    {
        EXPECT_EQ(pkt.to, ep1);
    }
}

// 测试 6: 无效数据包处理
TEST_F(KcpServerTest, InvalidPacket)
{
    asio::ip::udp::endpoint ep(asio::ip::make_address("127.0.0.1"), 10001);

    // 太短的包（小于 4 字节）
    std::vector<uint8_t> shortPacket = {1, 2};

    EXPECT_NO_THROW(m_server->testInput(ep, shortPacket));
    EXPECT_EQ(m_server->sessionCount(), 0);
}

// 测试 7: Update 不崩溃
TEST_F(KcpServerTest, UpdateWithoutSessions)
{
    // 没有任何 Session 时调用 update
    EXPECT_NO_THROW(m_server->testUpdate(0));
    EXPECT_NO_THROW(m_server->testUpdate(1000));
}

// ========== Client 测试助手类 ==========
class TestClient : public Client
{
public:
    explicit TestClient(IUdpTransport& transport, asio::any_io_executor exec) : Client(transport, std::move(exec)) {}

    void testInput(const asio::ip::udp::endpoint& from, std::span<const uint8_t> data) { input(from, data); }
};

// ========== Client 测试 Fixture ==========
class KcpClientTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_ioc = std::make_unique<asio::io_context>();
        m_transport = std::make_unique<MockUdpTransport>();
        m_client = std::make_unique<TestClient>(*m_transport, m_ioc->get_executor());
    }

    void TearDown() override
    {
        m_client.reset();
        m_transport.reset();
        m_ioc.reset();
    }

    std::unique_ptr<asio::io_context> m_ioc;
    std::unique_ptr<MockUdpTransport> m_transport;
    std::unique_ptr<TestClient> m_client;
};

// 测试 1: 创建 Client
TEST_F(KcpClientTest, CreateClient)
{
    ASSERT_NE(m_client, nullptr);
}

// 测试 2: 连接服务器
TEST_F(KcpClientTest, ConnectToServer)
{
    asio::ip::udp::endpoint serverEp(asio::ip::make_address("127.0.0.1"), 8888);
    uint32_t conv = 1001;

    auto session = m_client->connect(conv, serverEp);

    ASSERT_NE(session, nullptr);
    // KcpSession 没有 getConv() 方法，通过非空指针验证
}

// 测试 3: 多次连接相同 Conv 返回相同 Session
TEST_F(KcpClientTest, ReuseConnectionSession)
{
    asio::ip::udp::endpoint serverEp(asio::ip::make_address("127.0.0.1"), 8888);
    uint32_t conv = 2001;

    auto session1 = m_client->connect(conv, serverEp);
    auto session2 = m_client->connect(conv, serverEp);

    EXPECT_EQ(session1, session2); // 应该返回同一个对象
}

// 测试 4: 不同 Conv 创建不同 Session
TEST_F(KcpClientTest, DifferentConvDifferentSession)
{
    asio::ip::udp::endpoint serverEp(asio::ip::make_address("127.0.0.1"), 8888);

    auto session1 = m_client->connect(3001, serverEp);
    auto session2 = m_client->connect(3002, serverEp);

    EXPECT_NE(session1, session2);
    // KcpSession 没有 getConv() 方法，通过指针不同验证
}

// 测试 5: 客户端接收数据包
TEST_F(KcpClientTest, ReceivePacket)
{
    asio::ip::udp::endpoint serverEp(asio::ip::make_address("127.0.0.1"), 8888);
    uint32_t conv = 4001;

    auto session = m_client->connect(conv, serverEp);

    // 模拟接收来自服务器的数据包
    std::vector<uint8_t> packet(24, 0);
    std::memcpy(packet.data(), &conv, sizeof(conv));

    EXPECT_NO_THROW(m_client->testInput(serverEp, packet));
}

// 测试 6: 客户端发送数据
TEST_F(KcpClientTest, SendData)
{
    asio::ip::udp::endpoint serverEp(asio::ip::make_address("127.0.0.1"), 8888);
    auto session = m_client->connect(5001, serverEp);

    m_transport->clearPackets();

    std::vector<uint8_t> testData = {0xAA, 0xBB, 0xCC};
    session->send(testData);
    session->update(0);

    // 验证数据发送到了服务器
    EXPECT_GT(m_transport->getSendCount(), 0);
    EXPECT_TRUE(m_transport->hasPacketTo(serverEp));
}
