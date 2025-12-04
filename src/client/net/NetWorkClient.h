/**
 * ************************************************************************
 *
 * @file NetWorkClient.h
 * @author AnakinLiu
 * @date 2025-12-02
 * @version 0.3
 * @brief  基于 ASIO + C++23 协程的 UDP 网络管理器
 *         - 完全由 ThreadPool 驱动
 *         - 支持异步发送/接收
 *         - 支持可靠传输（ACK机制，事件驱动）
 *         - 客户端模式
 *
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
#include "src/shared/messages/PacketHeader.h"
#include "src/client/utils/ThreadPool.h"
#include "src/client/utils/Logger.h"

constexpr uint16_t ACK_PACKET_TYPE = 0xFFFF;
constexpr size_t MAX_PACKET_SIZE = 1024;
constexpr int DEFAULT_ACK_TIMEOUT_MS = 1000;

class NetWorkClient : public std::enable_shared_from_this<NetWorkClient>
{
public:
    explicit NetWorkClient() = default;

    ~NetWorkClient() noexcept
    {
        try
        {
            stop();
        }
        catch (...)
        {
            // 析构函数不应抛出异常
        }
    }

    void start()
    {
        if (m_running.exchange(true)) return;

        try
        {
            // 创建 socket 并打开
            m_socket.emplace(m_ioContext);
            m_socket->open(asio::ip::udp::v4());

            // 绑定到任意可用端口
            m_socket->bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), 0));

            utils::LOG_INFO("Network client started, socket ready on port {}", m_socket->local_endpoint().port());

            // 启动 receiveLoop 协程（在 m_ioContext 上运行）
            auto self = shared_from_this();
            asio::co_spawn(
                m_ioContext,
                // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines)
                [self]() -> asio::awaitable<void> { co_await self->receiveLoop(); },
                asio::detached);

            // 启动 io_context 在 ThreadPool 上执行
            asio::post(utils::ThreadPool::getInstance().get_executor(), [self] { self->m_ioContext.run(); });

            // 等待网络层完全启动
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        catch (const std::exception& e)
        {
            utils::LOG_ERROR("Failed to start network client: {}", e.what());
            m_running.store(false);
            throw;
        }
    }

    void stop() noexcept
    {
        try
        {
            bool expected = true;
            if (!m_running.compare_exchange_strong(expected, false))
            {
                return;
            }

            if (m_socket)
            {
                asio::error_code cancelError;
                [[maybe_unused]] auto cancelResult = m_socket->cancel(cancelError);
                if (cancelError)
                {
                    utils::LOG_WARN("Socket cancel failed: {}", cancelError.message());
                }

                asio::error_code closeError;
                [[maybe_unused]] auto closeResult = m_socket->close(closeError);
                if (closeError)
                {
                    utils::LOG_WARN("Socket close failed: {}", closeError.message());
                }

                m_socket.reset();
            }

            // 清理所有待处理的 ACK
            {
                std::scoped_lock lock(m_ackMutex);
                m_pendingAcks.clear();
            }

            m_ioContext.stop();
        }
        catch (const std::exception& e)
        {
            // stop 应当是 noexcept，记录但不抛出
            utils::LOG_ERROR("Stop failed: {}", e.what());
        }
        catch (...)
        {
            utils::LOG_ERROR("Stop failed with unknown exception");
        }
    }

    NetWorkClient(const NetWorkClient&) = delete;
    NetWorkClient& operator=(const NetWorkClient&) = delete;
    NetWorkClient(NetWorkClient&&) = delete;
    NetWorkClient& operator=(NetWorkClient&&) = delete;
    /**
     * @brief 连接到服务器
     * @param host 服务器主机名或IP地址
     * @param port 服务器端口号
     */
    void connect(const std::string& host, uint16_t port)
    {
        asio::ip::udp::resolver resolver(utils::ThreadPool::getInstance().get_executor());
        auto endpoints = resolver.resolve(asio::ip::udp::v4(), host, std::to_string(port));
        m_serverEndpoint = *endpoints.begin();
    }
    void disconnect()
    {
        if (!m_running.load()) return; // 已经停止或未启动

        if (m_socket)
        {
            asio::error_code errorCode;
            [[maybe_unused]] auto closeResult = m_socket->close(errorCode);
            if (errorCode) utils::LOG_WARN("Disconnect failed: {}", errorCode.message());
        }

        // 可选：清空服务器 endpoint
        m_serverEndpoint = asio::ip::udp::endpoint{};
    }

    void setPacketHandler(std::function<void(uint16_t, const uint8_t*, size_t, const asio::ip::udp::endpoint&)> handler)
    {
        m_packetHandler = std::move(handler);
    }

    // 不可靠发送
    asio::awaitable<void> sendPacket(uint16_t type, std::vector<uint8_t> payload)
    {
        if (!m_running.load())
        {
            utils::LOG_ERROR("Cannot send packet: network not running");
            co_return;
        }

        utils::LOG_INFO("[Client] sendPacket type={}, size={} bytes", type, payload.size());

        auto data = buildPacket(type, 0, payload);
        asio::error_code errorCode;
        size_t sent =
            co_await m_socket->async_send_to(asio::buffer(*data),
                                             m_serverEndpoint,
                                             asio::bind_executor(utils::ThreadPool::getInstance().get_executor(),
                                                                 asio::redirect_error(asio::use_awaitable, errorCode)));

        if (errorCode)
        {
            utils::LOG_ERROR("Send packet failed: {}", errorCode.message());
        }
        else
        {
            utils::LOG_DEBUG(
                "Sent {} bytes to {}:{}", sent, m_serverEndpoint.address().to_string(), m_serverEndpoint.port());
        }
    }

    // 可靠发送
    asio::awaitable<bool>
        sendReliablePacket(uint16_t type,
                           std::vector<uint8_t> payload,
                           int maxRetries = 3,
                           std::chrono::milliseconds timeout = std::chrono::milliseconds(DEFAULT_ACK_TIMEOUT_MS))
    {
        if (!m_running.load()) co_return false;

        uint32_t seq = m_nextSeq.fetch_add(1, std::memory_order_relaxed);
        auto data = buildPacket(type, seq, payload);

        utils::LOG_INFO("[Client] Sending reliable packet: type={}, seq={}, size={}", type, seq, payload.size());

        for (int retry = 0; retry < maxRetries; ++retry)
        {
            if (!m_running.load()) co_return false;

            asio::error_code errorCode;
            size_t sent = co_await m_socket->async_send_to(
                asio::buffer(*data),
                m_serverEndpoint,
                asio::bind_executor(utils::ThreadPool::getInstance().get_executor(),
                                    asio::redirect_error(asio::use_awaitable, errorCode)));

            if (errorCode)
            {
                utils::LOG_ERROR("Failed to send reliable packet (retry {}): {}", retry, errorCode.message());
                co_return false;
            }

            utils::LOG_DEBUG("Sent {} bytes (seq={}), waiting for ACK... (retry {})", sent, seq, retry);

            bool ack = co_await waitForAck(seq, timeout);
            if (ack)
            {
                utils::LOG_DEBUG("Received ACK for seq={}", seq);
                co_return true;
            }
            utils::LOG_WARN("Timeout waiting for ACK (seq={}, retry={})", seq, retry);
        }

        utils::LOG_ERROR("Failed to send reliable packet after {} retries", maxRetries);
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
            // 使用偏移量而非指针算术
            std::memcpy(&(*data)[sizeof(PacketHeader)], payload.data(), payload.size());
        }
        return data;
    }

    // 等待单个 ACK (事件驱动 + 超时)
    asio::awaitable<bool> waitForAck(uint32_t seq, std::chrono::milliseconds timeout)
    {
        auto timer = std::make_shared<asio::steady_timer>(utils::ThreadPool::getInstance().get_executor());
        timer->expires_after(timeout);

        {
            std::scoped_lock lock(m_ackMutex);
            m_pendingAcks[seq] = timer;
        }

        asio::error_code errorCode;
        co_await timer->async_wait(asio::redirect_error(asio::use_awaitable, errorCode));

        bool ackReceived = false;
        {
            std::scoped_lock lock(m_ackMutex);
            ackReceived = !m_pendingAcks.contains(seq);
            m_pendingAcks.erase(seq);
        }

        co_return ackReceived;
    }

    asio::awaitable<void> receiveLoop()
    {
        utils::LOG_INFO("Receive loop started");
        std::array<uint8_t, MAX_PACKET_SIZE> buf{};
        asio::ip::udp::endpoint sender;

        while (m_running.load())
        {
            asio::error_code errorCode;
            std::size_t bytes = co_await m_socket->async_receive_from(
                asio::buffer(buf),
                sender,
                asio::bind_executor(utils::ThreadPool::getInstance().get_executor(),
                                    asio::redirect_error(asio::use_awaitable, errorCode)));

            if (errorCode)
            {
                if (errorCode != asio::error::operation_aborted)
                {
                    utils::LOG_ERROR("Receive error: {}", errorCode.message());
                }
                break;
            }

            utils::LOG_DEBUG("Received {} bytes from {}:{}", bytes, sender.address().to_string(), sender.port());

            if (bytes < sizeof(PacketHeader))
            {
                utils::LOG_WARN("Packet too small: {} bytes", bytes);
                continue;
            }

            PacketHeader header{};
            std::memcpy(&header, buf.data(), sizeof(PacketHeader));

            utils::LOG_INFO("[Client] Received packet: type={}, seq={}, ack={}, payloadSize={}",
                            header.type,
                            header.seq,
                            header.ack,
                            header.size);

            // 处理 ACK
            if (header.type == ACK_PACKET_TYPE && header.ack != 0)
            {
                std::shared_ptr<asio::steady_timer> timer;
                {
                    std::scoped_lock lock(m_ackMutex);
                    auto iter = m_pendingAcks.find(header.ack);
                    if (iter != m_pendingAcks.end())
                    {
                        timer = iter->second;
                        m_pendingAcks.erase(iter);
                    }
                }
                if (timer) asio::post(utils::ThreadPool::getInstance().get_executor(), [timer]() { timer->cancel(); });
                continue;
            }

            // 收到可靠包时发送 ACK
            if (header.seq != 0)
            {
                utils::LOG_DEBUG("[Client] Received reliable packet with seq={}, sending ACK", header.seq);
                asio::co_spawn(
                    utils::ThreadPool::getInstance().get_executor(),
                    // NOLINTNEXTLINE(cppcoreguidelines-avoid-capturing-lambda-coroutines) - safe: shared_ptr ensures
                    [self = shared_from_this(), seq = header.seq]() -> asio::awaitable<void>
                    { co_await self->sendAck(seq); },
                    asio::detached);
            }

            // 调用用户 handler
            if (m_packetHandler)
            {
                asio::post(utils::ThreadPool::getInstance().get_executor(),
                           [self = shared_from_this(),
                            buf,
                            type = header.type,
                            size = header.size,
                            sender,
                            handler = m_packetHandler]()
                           {
                               // 传递消息类型和 Payload
                               handler(type, &buf[sizeof(PacketHeader)], size, sender);
                           });
            }
        }

        m_running.store(false);
    }

    asio::awaitable<void> sendAck(uint32_t seq)
    {
        if (!m_running.load())
        {
            co_return;
        }
        PacketHeader ack{.seq = 0, .ack = seq, .type = ACK_PACKET_TYPE, .size = 0};
        std::array<uint8_t, sizeof(PacketHeader)> ackBuf{};
        std::memcpy(ackBuf.data(), &ack, sizeof(PacketHeader));

        asio::error_code errorCode;
        size_t sent =
            co_await m_socket->async_send_to(asio::buffer(ackBuf),
                                             m_serverEndpoint,
                                             asio::bind_executor(utils::ThreadPool::getInstance().get_executor(),
                                                                 asio::redirect_error(asio::use_awaitable, errorCode)));

        if (errorCode)
        {
            utils::LOG_ERROR("[Client] Failed to send ACK for seq={}: {}", seq, errorCode.message());
        }
        else
        {
            utils::LOG_DEBUG("[Client] Sent ACK for seq={}, {} bytes", seq, sent);
        }
    }

    asio::io_context m_ioContext;
    std::optional<asio::ip::udp::socket> m_socket;
    std::atomic<bool> m_running{false};

    asio::ip::udp::endpoint m_serverEndpoint;
    std::atomic<uint32_t> m_nextSeq{1};

    std::function<void(uint16_t, const uint8_t*, size_t, const asio::ip::udp::endpoint&)> m_packetHandler;

    // 多可靠包 ACK 管理
    std::unordered_map<uint32_t, std::shared_ptr<asio::steady_timer>> m_pendingAcks;
    std::mutex m_ackMutex; // 保护 pendingAcks
};
