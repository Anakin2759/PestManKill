/**
 * @file KcpClient.h
 */
#pragma once
#include "KcpEndpoint.h"

class Client final : public KcpEndpoint
{
public:
    explicit Client(IUdpTransport& transport) : KcpEndpoint(transport) {}

    /**
     * @brief 主动连接服务器
     * @param conv 预先分配的会话 ID
     * @param server_ep 服务器的 UDP 地址
     * @return 建立好的会话对象
     */
    std::shared_ptr<KcpSession> connect(uint32_t conv, const asio::ip::udp::endpoint& server_ep)
    {
        auto [it, inserted] = m_sessions.try_emplace(conv);
        if (inserted)
        {
            it->second = std::make_shared<KcpSession>(conv, m_transport, server_ep);
            // 客户端手动标记活跃，防止刚创建就被 update 逻辑清理
            // 这里假设你沿用了原代码的 m_lastActive 结构
            // m_lastActive[conv] = std::chrono::steady_clock::now();

            onSession(conv, it->second);
        }
        return it->second;
    }

protected:
    /**
     * @brief 客户端的识别逻辑通常比较简单
     * 因为客户端通常只跟一个服务器通信，直接解析包里的 conv 即可
     */
    uint32_t selectConv(const asio::ip::udp::endpoint&, std::span<const uint8_t> data) override
    {
        return peekConv(data);
    }
};