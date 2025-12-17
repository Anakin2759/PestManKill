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

class AsioUdpTransport final : public IUdpTransport
{
public:
    AsioUdpTransport(asio::io_context& ctx, uint16_t port);

    void send(const asio::ip::udp::endpoint& to, std::span<const uint8_t> data) override;

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
    asio::ip::udp::socket m_socket;
};