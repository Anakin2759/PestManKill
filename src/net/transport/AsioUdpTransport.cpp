#include "src/net/transport/AsioUdpTransport.h"

#include <asio.hpp>

AsioUdpTransport::AsioUdpTransport(asio::io_context& ctx, uint16_t port)
    : m_socket(ctx, asio::ip::udp::endpoint(asio::ip::udp::v4(), port))
{
}

void AsioUdpTransport::send(const asio::ip::udp::endpoint& to, std::span<const uint8_t> data)
{
    m_socket.send_to(asio::buffer(data.data(), data.size()), to);
}