#include "Client.h"

#include <chrono>
#include <memory>
#include <utility>

Client::Client(IUdpTransport& transport, asio::any_io_executor exec) : KcpEndpoint(std::move(exec), transport) {}

std::shared_ptr<KcpSession> Client::connect(uint32_t conv, const asio::ip::udp::endpoint& server_ep)
{
    auto [iter, inserted] = m_sessions.try_emplace(conv);
    if (inserted)
    {
        // 使用基类持有的 m_exec 创建 Session
        iter->second = std::make_shared<KcpSession>(conv, m_transport, server_ep, m_exec);

        // 初始化活跃时间
        m_lastActive[conv] = std::chrono::steady_clock::now();

        // 触发回调（如果业务层需要感知）
        onSession(conv, iter->second);
    }
    return iter->second;
}

uint32_t Client::selectConv([[maybe_unused]] const asio::ip::udp::endpoint& from, std::span<const uint8_t> data)
{
    return peekConv(data);
}
