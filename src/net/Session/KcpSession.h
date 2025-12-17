#pragma once
#include "../transport/IUdpTransport.h"
#include <ikcp.h>
#include <asio.hpp>
#include <asio/experimental/channel.hpp>
#include <expected>
#include <span>
#include <memory>
#include <vector>

class KcpSession : public std::enable_shared_from_this<KcpSession>
{
    // 定义协程通道：传递接收到的二进制包
    using Packet = std::vector<uint8_t>;
    using DataChannel = asio::experimental::channel<asio::any_io_executor, void(std::error_code, Packet)>;

public:
    KcpSession(uint32_t conv, IUdpTransport& transport, asio::ip::udp::endpoint peer, asio::any_io_executor exec)
        : m_transport(transport), m_peer(peer), m_channel(exec, 64) // 缓冲区容量 64
    {
        m_kcp = ikcp_create(conv, this);
        m_kcp->output = &KcpSession::kcpOutput;

        // 默认配置优化（可根据需求调整）
        ikcp_nodelay(m_kcp, 1, 10, 2, 1);
        m_kcp->rx_minrto = 10;
    }

    ~KcpSession()
    {
        if (m_kcp) ikcp_release(m_kcp);
    }

    // 禁止拷贝
    KcpSession(const KcpSession&) = delete;
    KcpSession& operator=(const KcpSession&) = delete;

    /**
     * @brief 供 Endpoint 调用：喂入底层 UDP 数据
     */
    void input(std::span<const uint8_t> data)
    {
        ikcp_input(m_kcp, reinterpret_cast<const char*>(data.data()), static_cast<long>(data.size()));

        // 尝试从 KCP 提取完整包并推入协程通道
        std::vector<uint8_t> buf(2048);
        int n = 0;
        while ((n = ikcp_recv(m_kcp, reinterpret_cast<char*>(buf.data()), static_cast<int>(buf.size()))) > 0)
        {
            buf.resize(n);
            // try_send 是非阻塞的，如果通道满了会返回 false
            m_channel.try_send(std::error_code{}, std::move(buf));
            buf.resize(2048);
        }
    }

    /**
     * @brief 协程接口：异步接收一个 KCP 完整包
     * @return C++23 std::expected，成功返回数据，失败返回错误码
     */
    asio::awaitable<std::expected<Packet, std::error_code>> recv()
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

    void send(std::span<const uint8_t> data)
    {
        ikcp_send(m_kcp, reinterpret_cast<const char*>(data.data()), static_cast<int>(data.size()));
    }
    /**
     * @brief 更新 KCP 状态，需定期调用
     * @param now 当前时间戳（毫秒）
     */
    void update(uint32_t now) { ikcp_update(m_kcp, now); }

    // 获取下一次更新的时间点
    uint32_t check(uint32_t now) const { return ikcp_check(m_kcp, now); }

private:
    static int kcpOutput(const char* buf, int len, ikcpcb* kcp, void* user)
    {
        auto* self = static_cast<KcpSession*>(user);
        self->m_transport.send(self->m_peer, std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(buf), len));
        return 0;
    }

private:
    ikcpcb* m_kcp{nullptr};
    IUdpTransport& m_transport;
    asio::ip::udp::endpoint m_peer;
    DataChannel m_channel;
};