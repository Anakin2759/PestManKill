/**
 * ************************************************************************
 *
 * @file AsioUdpTransport.cpp
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

#include "AsioUdpTransport.h"
#include <asio.hpp>

AsioUdpTransport::AsioUdpTransport(asio::any_io_executor exec, uint16_t port)
    : m_socket(exec, asio::ip::udp::endpoint(asio::ip::udp::v4(), port))
{
}

void AsioUdpTransport::send(const asio::ip::udp::endpoint& to, std::span<const uint8_t> data)
{
    m_socket.send_to(asio::buffer(data.data(), data.size()), to);
}

uint16_t AsioUdpTransport::localPort() const
{
    return m_socket.local_endpoint().port();
}

void AsioUdpTransport::stop()
{
    asio::error_code ec;
    m_socket.close(ec);
}