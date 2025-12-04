/**
 * ************************************************************************
 *
 * @file NetWorkManager.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-02
 * @version 0.1
 * @brief 网络管理器定义
 * 基于 ASIO + C++23 协程的 UDP 网络管理器
 * - 完全由 ThreadPool 驱动
 * - 支持异步发送/接收
 * - 支持可靠传输（ACK机制，事件驱动）
 * - 服务器模式
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
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <mutex>
#include "src/shared/messages/PacketHeader.h"
#include "src/server/context/GameContext.h"

constexpr uint16_t ACK_PACKET_TYPE = 0xFFFF;
constexpr size_t MAX_PACKET_SIZE = 1024;
constexpr int DEFAULT_ACK_TIMEOUT_MS = 1000;

class NetWorkManager : public std::enable_shared_from_this<NetWorkManager>
{
public:
    explicit NetWorkManager(GameContext& context) : m_context(context) {}

    ~NetWorkManager() { stop(); }

    /**
     * @brief 启动服务器并绑定到指定端口
     * @param port 监听端口
     */
    void start(uint16_t port)
    {
        if (m_running.exchange(true)) return;

        m_socket.emplace(m_ioContext, asio::ip::udp::endpoint(asio::ip::udp::v4(), port));

        auto self = shared_from_this();
        // 启动 receiveLoop 协程
        asio::co_spawn(
            m_context.threadPool.get_executor(),
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines) - safe: shared_ptr ensures lifetime
            [self]() -> asio::awaitable<void> { co_await self->receiveLoop(); },
            asio::detached);

        // 启动 io_context 在 ThreadPool 上
        asio::post(m_context.threadPool.get_executor(),
                   [self]
                   {
                       self->m_context.logger->info("[Server] io_context run begin");
                       self->m_ioContext.run();
                       self->m_context.logger->info("[Server] io_context run end");
                   });
    }

    void stop()
    {
        bool expected = true;
        if (!m_running.compare_exchange_strong(expected, false)) return;

        if (m_socket)
        {
            asio::error_code cancelError;
            [[maybe_unused]] auto cancelResult = m_socket->cancel(cancelError);
            if (cancelError)
            {
                m_context.logger->warn("Socket cancel failed: {}", cancelError.message());
            }

            asio::error_code closeError;
            [[maybe_unused]] auto closeResult = m_socket->close(closeError);
            if (closeError)
            {
                m_context.logger->warn("Socket close failed: {}", closeError.message());
            }
        }
        m_ioContext.stop();
        m_context.logger->info("服务器已停止");
    }

    NetWorkManager(const NetWorkManager&) = delete;
    NetWorkManager& operator=(const NetWorkManager&) = delete;
    NetWorkManager(NetWorkManager&&) = delete;
    NetWorkManager& operator=(NetWorkManager&&) = delete;

    void setPacketHandler(std::function<void(uint16_t, const uint8_t*, size_t, const asio::ip::udp::endpoint&)> handler)
    {
        m_packetHandler = std::move(handler);
    }

    // 不可靠发送到指定客户端
    asio::awaitable<void> sendPacket(asio::ip::udp::endpoint endpoint, uint16_t type, std::vector<uint8_t> payload)
    {
        if (!m_running.load()) co_return;

        auto data = buildPacket(type, 0, payload);
        asio::error_code errorCode;
        co_await m_socket->async_send_to(asio::buffer(*data),
                                         endpoint,
                                         asio::bind_executor(m_context.threadPool.get_executor(),
                                                             asio::redirect_error(asio::use_awaitable, errorCode)));
        if (errorCode)
        {
            m_context.logger->error("发送数据包失败: {}", errorCode.message());
        }
        else
        {
            m_context.logger->info("[Server] sendPacket type={} size={} bytes to {}:{}",
                                   type,
                                   payload.size(),
                                   endpoint.address().to_string(),
                                   endpoint.port());
        }
    }

    // 可靠发送到指定客户端
    asio::awaitable<bool>
        sendReliablePacket(asio::ip::udp::endpoint endpoint,
                           uint16_t type,
                           std::vector<uint8_t> payload,
                           int maxRetries = 3,
                           std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_ACK_TIMEOUT_MS))
    {
        if (!m_running.load()) co_return false;

        uint32_t seq = m_nextSeq.fetch_add(1, std::memory_order_relaxed);
        auto data = buildPacket(type, seq, payload);

        m_context.logger->info("[Server] Sending reliable packet: type={}, seq={}, size={} to {}:{}",
                               type,
                               seq,
                               payload.size(),
                               endpoint.address().to_string(),
                               endpoint.port());

        for (int retry = 0; retry < maxRetries; ++retry)
        {
            if (!m_running.load()) co_return false;

            asio::error_code errorCode;
            co_await m_socket->async_send_to(asio::buffer(*data),
                                             endpoint,
                                             asio::bind_executor(m_context.threadPool.get_executor(),
                                                                 asio::redirect_error(asio::use_awaitable, errorCode)));

            if (errorCode)
            {
                m_context.logger->error("可靠发送失败: {}", errorCode.message());
                co_return false;
            }

            bool ack = co_await waitForAck(seq, endpoint, timeout);
            if (ack)
            {
                m_context.logger->info("[Server] 收到ACK确认, seq={}", seq);
                co_return true;
            }
            m_context.logger->warn("[Server] 未收到ACK, 重试 {}/{}", retry + 1, maxRetries);
        }

        co_return false;
    }

private:
    static std::shared_ptr<std::vector<uint8_t>>
        buildPacket(uint16_t type, uint32_t seq, const std::vector<uint8_t>& payload)
    {
        auto data = std::make_shared<std::vector<uint8_t>>(sizeof(PacketHeader) + payload.size());
        PacketHeader header{.seq = seq, .ack = 0, .type = type, .size = static_cast<uint16_t>(payload.size())};
        std::memcpy(data->data(), &header, sizeof(PacketHeader));
        if (!payload.empty())
        {
            std::memcpy(&(*data)[sizeof(PacketHeader)], payload.data(), payload.size());
        }
        return data;
    }

    // 为每个客户端生成唯一的ACK键
    struct AckKey
    {
        uint32_t seq;
        asio::ip::udp::endpoint endpoint;

        bool operator==(const AckKey& other) const { return seq == other.seq && endpoint == other.endpoint; }
    };

    struct AckKeyHash
    {
        std::size_t operator()(const AckKey& key) const
        {
            std::size_t hash1 = std::hash<uint32_t>{}(key.seq);
            std::size_t hash2 = std::hash<unsigned short>{}(key.endpoint.port());
            return hash1 ^ (hash2 << 1U);
        }
    };

    // 等待单个 ACK (事件驱动 + 超时)
    asio::awaitable<bool> waitForAck(uint32_t seq, asio::ip::udp::endpoint endpoint, std::chrono::milliseconds timeout)
    {
        auto timer = std::make_shared<asio::steady_timer>(m_context.threadPool.get_executor());
        timer->expires_after(timeout);

        AckKey key{seq, endpoint};
        {
            std::scoped_lock lock(m_ackMutex);
            m_pendingAcks[key] = timer;
        }

        asio::error_code errorCode;
        co_await timer->async_wait(asio::redirect_error(asio::use_awaitable, errorCode));

        bool ackReceived = false;
        {
            std::scoped_lock lock(m_ackMutex);
            ackReceived = !m_pendingAcks.contains(key);
            m_pendingAcks.erase(key);
        }

        co_return ackReceived;
    }

    asio::awaitable<void> receiveLoop()
    {
        std::array<uint8_t, MAX_PACKET_SIZE> buf{};
        asio::ip::udp::endpoint sender;

        while (m_running.load())
        {
            asio::error_code errorCode;
            std::size_t bytes = co_await m_socket->async_receive_from(
                asio::buffer(buf),
                sender,
                asio::bind_executor(m_context.threadPool.get_executor(),
                                    asio::redirect_error(asio::use_awaitable, errorCode)));

            if (errorCode)
            {
                if (errorCode != asio::error::operation_aborted)
                {
                    m_context.logger->error("接收数据失败: {}", errorCode.message());
                }
                break;
            }

            if (bytes < sizeof(PacketHeader)) continue;

            PacketHeader header{};
            std::memcpy(&header, buf.data(), sizeof(PacketHeader));

            // 处理 ACK
            if (header.type == ACK_PACKET_TYPE && header.ack != 0)
            {
                std::shared_ptr<asio::steady_timer> timer;
                AckKey key{header.ack, sender};
                {
                    std::scoped_lock lock(m_ackMutex);
                    auto iter = m_pendingAcks.find(key);
                    if (iter != m_pendingAcks.end())
                    {
                        timer = iter->second;
                        m_pendingAcks.erase(iter);
                        m_context.logger->debug("[Server] Received ACK for seq={} from {}:{}",
                                                header.ack,
                                                sender.address().to_string(),
                                                sender.port());
                    }
                    else
                    {
                        m_context.logger->warn("[Server] Received unexpected ACK for seq={} from {}:{}",
                                               header.ack,
                                               sender.address().to_string(),
                                               sender.port());
                    }
                }
                if (timer)
                {
                    asio::post(m_context.threadPool.get_executor(), [timer]() { timer->cancel(); });
                }
                continue;
            }

            // 收到可靠包时发送 ACK
            if (header.seq != 0)
            {
                asio::co_spawn(
                    m_context.threadPool.get_executor(),
                    // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines) - safe: shared_ptr ensures
                    [self = shared_from_this(), seq = header.seq, sender]() -> asio::awaitable<void>
                    { co_await self->sendAck(seq, sender); },
                    asio::detached);
            }

            // 调用用户 handler
            if (header.size > bytes - sizeof(PacketHeader))
            {
                m_context.logger->warn(
                    "Payload size mismatch: header={}, actual={}", header.size, bytes - sizeof(PacketHeader));
                continue;
            }

            if (m_packetHandler)
            {
                asio::post(m_context.threadPool.get_executor(),
                           [self = shared_from_this(),
                            buf,
                            type = header.type,
                            size = header.size,
                            sender,
                            handler = m_packetHandler]() { handler(type, &buf[sizeof(PacketHeader)], size, sender); });
            }
        }

        m_running.store(false);
    }

    asio::awaitable<void> sendAck(uint32_t seq, asio::ip::udp::endpoint endpoint)
    {
        if (!m_running.load())
        {
            co_return;
        }
        PacketHeader ack{.seq = 0, .ack = seq, .type = ACK_PACKET_TYPE, .size = 0};
        std::array<uint8_t, sizeof(PacketHeader)> ackBuf{};
        std::memcpy(ackBuf.data(), &ack, sizeof(PacketHeader));

        asio::error_code errorCode;
        co_await m_socket->async_send_to(asio::buffer(ackBuf),
                                         endpoint,
                                         asio::bind_executor(m_context.threadPool.get_executor(),
                                                             asio::redirect_error(asio::use_awaitable, errorCode)));
        if (errorCode)
        {
            m_context.logger->error("发送ACK失败: {}", errorCode.message());
        }
    }

    GameContext& m_context;
    asio::io_context m_ioContext;
    std::optional<asio::ip::udp::socket> m_socket;
    std::atomic<bool> m_running{false};

    std::atomic<uint32_t> m_nextSeq{1};

    std::function<void(uint16_t, const uint8_t*, size_t, const asio::ip::udp::endpoint&)> m_packetHandler;

    // 多可靠包 ACK 管理（按客户端endpoint区分）
    std::unordered_map<AckKey, std::shared_ptr<asio::steady_timer>, AckKeyHash> m_pendingAcks;
    std::mutex m_ackMutex; // 保护 pendingAcks
};