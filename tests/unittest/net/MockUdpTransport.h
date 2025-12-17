/**
 * ************************************************************************
 *
 * @file MockUdpTransport.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-17
 * @version 0.1
 * @brief 模拟 UDP 传输实现（用于单元测试）
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once
#include "src/net/transport/IUdpTransport.h"
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "KcpServer.h"

// 假设你的 IUdpTransport 定义如下
class MockUdpTransport : public IUdpTransport
{
public:
    MOCK_METHOD(void, send, (const asio::ip::udp::endpoint& to, std::span<const uint8_t> data), (override));
};

// 为了测试回调，我们创建一个测试用的 Server 类
class TestServer : public Server
{
public:
    using Server::Server;

    // 记录回调状态
    struct SessionEvent
    {
        uint32_t conv;
        bool created = false;
        bool closed = false;
    };
    std::unordered_map<uint32_t, SessionEvent> events;

protected:
    void onSession(uint32_t conv, std::shared_ptr<KcpSession> sess) override
    {
        events[conv].conv = conv;
        events[conv].created = true;
    }

    void onSessionClosed(uint32_t conv) override { events[conv].closed = true; }
};
