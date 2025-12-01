/**
 * ************************************************************************
 *
 * @file NetWorkManager.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-01
 * @version 0.1
 * @brief 网络管理器定义
  基于ASIO实现的UDP网络通信管理器
  支持异步发送和接收数据包
  支持可靠传输机制
  基于cpp23协程实现异步操作
  客户端实现
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <asio.hpp>
#include <array>
#include <chrono>
#include <cstring>
#include <functional>
#include <memory>
#include <unordered_set>
#include <vector>
#include <thread>
#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/executor_work_guard.hpp>
#include "src/shared/messages/PacketHeader.h"

// 常量定义
constexpr uint16_t ACK_PACKET_TYPE = 0xFFFF;
constexpr size_t MAX_PACKET_SIZE = 1024;
constexpr int DEFAULT_ACK_POLL_MS = 10;
constexpr int DEFAULT_ACK_TIMEOUT_MS = 1000;

class NetWorkManager
{
public:
    explicit NetWorkManager()
        : m_socket(m_ioContext, asio::ip::udp::v4()), m_strand(asio::make_strand(m_ioContext)),
          m_workGuard(asio::make_work_guard(m_ioContext))
    {
        asio::co_spawn(m_ioContext, receiveLoop(), asio::detached);

        // 启动独立的工作线程来运行 io_context
        m_ioThread = std::thread([this]() { m_ioContext.run(); });
    }

    ~NetWorkManager()
    {
        // 停止 io_context 并等待线程结束
        m_workGuard.reset();
        m_ioContext.stop();
        if (m_ioThread.joinable())
        {
            m_ioThread.join();
        }
    }

    // 禁止拷贝和移动
    NetWorkManager(const NetWorkManager&) = delete;
    NetWorkManager& operator=(const NetWorkManager&) = delete;
    NetWorkManager(NetWorkManager&&) = delete;
    NetWorkManager& operator=(NetWorkManager&&) = delete;

    // 连接到服务器
    void connect(const std::string& host, uint16_t port)
    {
        asio::ip::udp::resolver resolver(m_ioContext);
        auto endpoints = resolver.resolve(asio::ip::udp::v4(), host, std::to_string(port));
        m_serverEndpoint = *endpoints.begin();
    }

    // 设置数据包处理器
    void setPacketHandler(std::function<void(const uint8_t*, size_t, const asio::ip::udp::endpoint&)> handler)
    {
        m_packetHandler = std::move(handler);
    }

    // 发送不可靠包
    asio::awaitable<void> sendPacket(uint16_t type, std::vector<uint8_t> payload)
    {
        PacketHeader header{.seq = 0, .ack = 0, .type = type, .size = static_cast<uint16_t>(payload.size())};
        std::vector<uint8_t> data(sizeof(PacketHeader) + payload.size());
        std::memcpy(data.data(), &header, sizeof(PacketHeader));
        if (!payload.empty())
        {
            std::memcpy(&data[sizeof(PacketHeader)], payload.data(), payload.size());
        }
        co_await m_socket.async_send_to(asio::buffer(data), m_serverEndpoint, asio::use_awaitable);
    }

    // 发送可靠包
    asio::awaitable<bool>
        sendReliablePacket(uint16_t type,
                           std::vector<uint8_t> payload,
                           int maxRetries = 3,
                           std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_ACK_TIMEOUT_MS))
    {
        uint32_t seq = m_nextSeq++;

        // 复用缓冲区
        auto data = std::make_shared<std::vector<uint8_t>>(sizeof(PacketHeader) + payload.size());
        PacketHeader header{.seq = seq, .ack = 0, .type = type, .size = static_cast<uint16_t>(payload.size())};
        std::memcpy(data->data(), &header, sizeof(PacketHeader));
        if (!payload.empty())
        {
            std::memcpy(&(*data)[sizeof(PacketHeader)], payload.data(), payload.size());
        }

        for (int retry = 0; retry < maxRetries; ++retry)
        {
            co_await m_socket.async_send_to(asio::buffer(*data), m_serverEndpoint, asio::use_awaitable);

            bool ackReceived = co_await waitForAck(seq, timeout);
            if (ackReceived)
            {
                co_return true;
            }
        }

        co_return false;
    }

private:
    // 单 timer 等待 ACK
    asio::awaitable<bool> waitForAck(uint32_t seq, std::chrono::milliseconds timeout)
    {
        auto start = std::chrono::steady_clock::now();
        asio::steady_timer timer(m_ioContext);

        while (true)
        {
            // 检查ACK
            if (m_receivedAcks.contains(seq))
            {
                m_receivedAcks.erase(seq);
                co_return true;
            }

            auto now = std::chrono::steady_clock::now();
            if (now - start >= timeout)
            {
                co_return false;
            }

            timer.expires_after(std::chrono::milliseconds(DEFAULT_ACK_POLL_MS));
            co_await timer.async_wait(asio::use_awaitable);
        }
    }

    asio::awaitable<void> receiveLoop()
    {
        std::array<uint8_t, MAX_PACKET_SIZE> buf{};
        asio::ip::udp::endpoint sender;

        for (;;)
        {
            std::size_t bytesReceived =
                co_await m_socket.async_receive_from(asio::buffer(buf), sender, asio::use_awaitable);

            if (bytesReceived < sizeof(PacketHeader))
            {
                continue;
            }

            PacketHeader header{};
            std::memcpy(&header, buf.data(), sizeof(PacketHeader));

            if (header.type == ACK_PACKET_TYPE && header.ack != 0)
            {
                asio::post(m_strand, [this, seq = header.ack]() { m_receivedAcks.insert(seq); });
                continue;
            }

            if (header.seq != 0)
            {
                asio::co_spawn(m_ioContext, sendAck(header.seq), asio::detached);
            }

            if (m_packetHandler)
            {
                m_packetHandler(&buf[sizeof(PacketHeader)], header.size, sender);
            }
        }
    }

    asio::awaitable<void> sendAck(uint32_t seq)
    {
        PacketHeader ack{.seq = 0, .ack = seq, .type = ACK_PACKET_TYPE, .size = 0};
        std::array<uint8_t, sizeof(PacketHeader)> ackBuf{};
        std::memcpy(ackBuf.data(), &ack, sizeof(PacketHeader));
        co_await m_socket.async_send_to(asio::buffer(ackBuf), m_serverEndpoint, asio::use_awaitable);
    }

    asio::io_context m_ioContext;
    asio::ip::udp::socket m_socket;
    asio::strand<asio::io_context::executor_type> m_strand;
    asio::executor_work_guard<asio::io_context::executor_type> m_workGuard;
    std::thread m_ioThread;

    asio::ip::udp::endpoint m_serverEndpoint;
    std::unordered_set<uint32_t> m_receivedAcks;
    uint32_t m_nextSeq{1};

    std::function<void(const uint8_t*, size_t, const asio::ip::udp::endpoint&)> m_packetHandler;
};
