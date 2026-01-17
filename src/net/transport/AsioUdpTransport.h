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
#include <span>
#include "IUdpTransport.h"
#include <array>
#include <concepts>
class AsioUdpTransport final : public IUdpTransport
{
public:
    AsioUdpTransport(asio::any_io_executor exec, uint16_t port);
    /**
     * @brief 发送数据包
     * @param to 目标地址
     * @param data 数据内容
     */
    void send(const asio::ip::udp::endpoint& endpoint, std::span<const uint8_t> data) override;

    /**
     * @brief 获取本地绑定端口
     */
    [[nodiscard]] uint16_t localPort() const;

    /**
     * @brief 停止传输
     */
    void stop();

    /**
     * @brief 协程接口：异步接收 UDP 包
     * @tparam PacketHandler 处理函数类型，签名应为 void(const asio::ip::udp::endpoint&, std::span<const uint8_t>)
     */
    template <typename PacketHandler>
        requires std::invocable<PacketHandler>
    asio::awaitable<void> recvLoop(PacketHandler&& handler)
    {
        std::array<uint8_t, 2048> buf{};
        asio::ip::udp::endpoint from;

        for (;;)
        {
            asio::error_code ec;
            std::size_t n = co_await m_socket.async_receive_from(
                asio::buffer(buf), from, asio::redirect_error(asio::use_awaitable, ec));
            if (ec)
            {
                co_return;
            }

            handler(from, std::span<const uint8_t>(buf.data(), n));
        }
    }

private:
    asio::ip::udp::socket m_socket;
};