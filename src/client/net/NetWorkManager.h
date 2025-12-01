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
#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/thread_pool.hpp>
#include "src/shared/messages/PacketHeader.h"
#include "src/client/utils/ThreadPool.h"
// 常量定义
constexpr uint16_t ACK_PACKET_TYPE = 0xFFFF;
constexpr size_t MAX_PACKET_SIZE = 1024;
constexpr int DEFAULT_ACK_POLL_MS = 10;
constexpr int DEFAULT_ACK_TIMEOUT_MS = 1000;

// 使用示例:
// auto manager = std::make_shared<NetWorkManager>(4); // 使用4个线程
// manager->start();
// manager->connect("127.0.0.1", 8080);
// manager->setPacketHandler(...);
// 使用完毕后调用 manager->stop() 或直接销毁对象
class NetWorkManager : public std::enable_shared_from_this<NetWorkManager>
{
public:
    explicit NetWorkManager()
        : m_threadPool(utils::ThreadPool::getInstance()), m_socket(m_ioContext, asio::ip::udp::v4()),
          m_strand(asio::make_strand(m_ioContext))
    {
    }

    // 初始化方法,必须在构造后立即调用(必须使用 shared_ptr 管理对象)
    void start()
    {
        if (m_running.exchange(true, std::memory_order_acq_rel))
        {
            return; // 已经启动
        }

        // 使用 shared_from_this 确保协程生命周期安全
        auto self = shared_from_this();
        asio::co_spawn(
            m_ioContext, [self]() -> asio::awaitable<void> { co_await self->receiveLoop(); }, asio::detached);

        // 在线程池中运行 io_context
        asio::post(m_threadPool, [this]() { m_ioContext.run(); });
    }

    ~NetWorkManager() { stop(); }

    // 显式停止方法
    void stop()
    {
        // 设置停止标志,让 receiveLoop 优雅退出
        bool expected = true;
        if (!m_running.compare_exchange_strong(expected, false, std::memory_order_acq_rel))
        {
            return; // 已经停止
        }

        // 关闭 socket,中断所有挂起的异步操作
        asio::error_code err;
        m_socket.close(err);

        // 停止 io_context
        m_ioContext.stop();

        // 等待线程池中的所有任务完成
        m_threadPool.join();
    }

    // 禁止拷贝和移动
    NetWorkManager(const NetWorkManager&) = delete;
    NetWorkManager& operator=(const NetWorkManager&) = delete;
    NetWorkManager(NetWorkManager&&) = delete;
    NetWorkManager& operator=(NetWorkManager&&) = delete;

    // 连接到服务器
    void connect(const std::string& host, uint16_t port)
    {
        asio::ip::udp::resolver resolver(m_threadPool.get_executor());
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
        if (!m_running.load(std::memory_order_acquire))
        {
            co_return;
        }

        PacketHeader header{.seq = 0, .ack = 0, .type = type, .size = static_cast<uint16_t>(payload.size())};
        std::vector<uint8_t> data(sizeof(PacketHeader) + payload.size());
        std::memcpy(data.data(), &header, sizeof(PacketHeader));
        if (!payload.empty())
        {
            std::memcpy(&data[sizeof(PacketHeader)], payload.data(), payload.size());
        }

        asio::error_code ec;
        co_await m_socket.async_send_to(
            asio::buffer(data), m_serverEndpoint, asio::redirect_error(asio::use_awaitable, ec));
    }

    // 发送可靠包
    asio::awaitable<bool>
        sendReliablePacket(uint16_t type,
                           std::vector<uint8_t> payload,
                           int maxRetries = 3,
                           std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_ACK_TIMEOUT_MS))
    {
        if (!m_running.load(std::memory_order_acquire))
        {
            co_return false;
        }

        uint32_t seq = m_nextSeq.fetch_add(1, std::memory_order_relaxed);

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
            if (!m_running.load(std::memory_order_acquire))
            {
                co_return false;
            }

            asio::error_code ec;
            co_await m_socket.async_send_to(
                asio::buffer(*data), m_serverEndpoint, asio::redirect_error(asio::use_awaitable, ec));

            if (ec)
            {
                co_return false;
            }

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

        while (m_running.load(std::memory_order_acquire))
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
            asio::error_code err;
            co_await timer.async_wait(asio::redirect_error(asio::use_awaitable, err));

            if (err)
            {
                co_return false;
            }
        }

        co_return false;
    }

    asio::awaitable<void> receiveLoop()
    {
        std::array<uint8_t, MAX_PACKET_SIZE> buf{};
        asio::ip::udp::endpoint sender;

        while (m_running.load(std::memory_order_acquire))
        {
            asio::error_code ec;
            std::size_t bytesReceived = co_await m_socket.async_receive_from(
                asio::buffer(buf), sender, asio::redirect_error(asio::use_awaitable, ec));

            // socket 关闭或错误,退出循环
            if (ec)
            {
                break;
            }

            if (bytesReceived < sizeof(PacketHeader))
            {
                continue;
            }

            PacketHeader header{};
            std::memcpy(&header, buf.data(), sizeof(PacketHeader));

            if (header.type == ACK_PACKET_TYPE && header.ack != 0)
            {
                auto self = shared_from_this();
                asio::post(m_strand, [self, seq = header.ack]() { self->m_receivedAcks.insert(seq); });
                continue;
            }

            if (header.seq != 0)
            {
                auto self = shared_from_this();
                asio::co_spawn(
                    m_ioContext,
                    [self, seq = header.seq]() -> asio::awaitable<void> { co_await self->sendAck(seq); },
                    asio::detached);
            }

            if (m_packetHandler)
            {
                m_packetHandler(&buf[sizeof(PacketHeader)], header.size, sender);
            }
        }

        m_running.store(false, std::memory_order_release);
    }

    asio::awaitable<void> sendAck(uint32_t seq)
    {
        if (!m_running.load(std::memory_order_acquire))
        {
            co_return;
        }

        PacketHeader ack{.seq = 0, .ack = seq, .type = ACK_PACKET_TYPE, .size = 0};
        std::array<uint8_t, sizeof(PacketHeader)> ackBuf{};
        std::memcpy(ackBuf.data(), &ack, sizeof(PacketHeader));

        asio::error_code ec;
        co_await m_socket.async_send_to(
            asio::buffer(ackBuf), m_serverEndpoint, asio::redirect_error(asio::use_awaitable, ec));
    }

    asio::thread_pool& m_threadPool;
    asio::io_context m_ioContext;
    asio::ip::udp::socket m_socket;
    asio::strand<asio::io_context::executor_type> m_strand;
    std::atomic<bool> m_running{false};

    asio::ip::udp::endpoint m_serverEndpoint;
    std::unordered_set<uint32_t> m_receivedAcks;
    std::atomic<uint32_t> m_nextSeq{1};

    std::function<void(const uint8_t*, size_t, const asio::ip::udp::endpoint&)> m_packetHandler;
};
