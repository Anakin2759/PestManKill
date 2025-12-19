/**
 * ************************************************************************
 *
 * @file AsioUdpTransport.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-17
 * @version 0.1
 * @brief ASIO UDP 层实现
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "IUdpTransport.h"
#include <array>
#include <functional>

class AsioUdpTransport final : public IUdpTransport
{
public:
    AsioUdpTransport(asio::any_io_executor exec, uint16_t port);

    void send(const asio::ip::udp::endpoint& to, std::span<const uint8_t> data) override;

    /**
     * @brief 获取本地绑定端口
     */
    [[nodiscard]] uint16_t localPort() const;

    /**
     * @brief 停止传输
     */
    void stop();

    /**
     * @brief 开始接收数据（非协程版本，用于测试）
     * @param handler 数据处理回调
     */
    template <typename PacketHandler>
    void startReceive(PacketHandler&& handler)
    {
        doReceive(std::forward<PacketHandler>(handler));
    }

    /**
     * @brief 协程接口：异步接收 UDP 包
     * @tparam PacketHandler 处理函数类型，签名应为 void(const asio::ip::udp::endpoint&, std::span<const uint8_t>)
     */
    template <typename PacketHandler>
    asio::awaitable<void> recvLoop(PacketHandler&& handler)
    {
        std::array<uint8_t, 2048> buf{};
        asio::ip::udp::endpoint from;

        for (;;)
        {
            std::size_t n = co_await m_socket.async_receive_from(asio::buffer(buf), from, asio::use_awaitable);

            handler(from, std::span<const uint8_t>(buf.data(), n));
        }
    }

private:
    template <typename PacketHandler>
    void doReceive(PacketHandler&& handler)
    {
        m_socket.async_receive_from(asio::buffer(m_recvBuffer),
                                    m_recvEndpoint,
                                    [this, handler = std::forward<PacketHandler>(handler)](const asio::error_code& ec,
                                                                                           std::size_t bytes) mutable
                                    {
                                        if (!ec && bytes > 0)
                                        {
                                            handler(m_recvEndpoint,
                                                    std::span<const uint8_t>(m_recvBuffer.data(), bytes));
                                            doReceive(std::move(handler));
                                        }
                                    });
    }

    asio::ip::udp::socket m_socket;
    std::array<uint8_t, 2048> m_recvBuffer{};
    asio::ip::udp::endpoint m_recvEndpoint;
};