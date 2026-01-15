/**
 * ************************************************************************
 *
 * @file KcpSession.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.1
 * @brief 会话层管理 KCP 连接
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#include "KcpSession.h"

#include <system_error>
#include <cstdint>
#include <utility>
#include <span>
#include <vector>

extern "C"
{
#include <ikcp.h>
}

namespace
{
constexpr size_t CHANNEL_CAPACITY = 64;
constexpr int KCP_UPDATE_INTERVAL_MS = 10;
constexpr int KCP_MIN_RTO_MS = 10;
constexpr size_t RECV_BUFFER_SIZE = 2048;
} // namespace

KcpSession::KcpSession(uint32_t conv,
                       IUdpTransport& transport,
                       asio::ip::udp::endpoint peer,
                       const asio::any_io_executor& exec)
    : m_kcp(ikcp_create(conv, this)), m_transport(transport), m_peer(std::move(peer)), m_channel(exec, CHANNEL_CAPACITY)
{
    if (m_kcp != nullptr)
    {
        m_kcp->output = &KcpSession::kcpOutput;
        ikcp_nodelay(m_kcp, 1, KCP_UPDATE_INTERVAL_MS, 2, 1);
        m_kcp->rx_minrto = KCP_MIN_RTO_MS;
    }
}

KcpSession::~KcpSession()
{
    if (m_kcp != nullptr)
    {
        ikcp_release(m_kcp);
    }
}

/**
 * @brief 输入数据到 KCP
 * @param data 输入数据
 */
void KcpSession::input(std::span<const uint8_t> data)
{
    if (m_kcp == nullptr)
    {
        return;
    }

    // KCP C API uses `char*` as byte buffer.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    ikcp_input(m_kcp, reinterpret_cast<const char*>(data.data()), static_cast<long>(data.size()));

    // 尝试从 KCP 提取完整包并推入协程通道
    std::vector<uint8_t> recvBuffer(RECV_BUFFER_SIZE);
    int bytesReceived = 0;
    while ((bytesReceived = ikcp_recv(m_kcp,
                                      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                                      reinterpret_cast<char*>(recvBuffer.data()),
                                      static_cast<int>(recvBuffer.size()))) > 0)
    {
        recvBuffer.resize(static_cast<size_t>(bytesReceived));
        m_channel.try_send(std::error_code{}, recvBuffer);
        recvBuffer.clear();
        recvBuffer.resize(RECV_BUFFER_SIZE);
    }
}

asio::awaitable<std::expected<KcpSession::Packet, std::error_code>> KcpSession::recv()
{
    try
    {
        Packet data = co_await m_channel.async_receive(asio::use_awaitable);
        co_return data;
    }
    catch (const std::system_error& e)
    {
        co_return std::unexpected(e.code());
    }
}

void KcpSession::send(std::span<const uint8_t> data)
{
    if (m_kcp == nullptr)
    {
        return;
    }
    // KCP C API uses `char*` as byte buffer.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    ikcp_send(m_kcp, reinterpret_cast<const char*>(data.data()), static_cast<int>(data.size()));
}

void KcpSession::update(uint32_t now)
{
    if (m_kcp != nullptr)
    {
        ikcp_update(m_kcp, now);
    }
}

uint32_t KcpSession::check(uint32_t now) const
{
    if (m_kcp == nullptr)
    {
        return now;
    }
    return ikcp_check(m_kcp, now);
}

int KcpSession::kcpOutput(const char* buf, int len, ikcpcb* /*kcp*/, void* user)
{
    auto* self = static_cast<KcpSession*>(user);
    if (self == nullptr)
    {
        return -1;
    }

    if (buf == nullptr || len <= 0)
    {
        return 0;
    }

    self->m_transport.send(self->m_peer,
                           // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                           std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(buf), static_cast<size_t>(len)));
    return 0;
}