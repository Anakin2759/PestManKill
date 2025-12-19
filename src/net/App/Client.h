#pragma once
#include "KcpEndpoint.h"
#include <asio.hpp>
#include <memory>
#include <utility>
#include "PeekConv.h"
class Client : public KcpEndpoint
{
public:
    /**
     * @brief 构造函数
     * @param transport UDP 传输层实现
     * @param exec 执行器（通常是 ioc.get_executor()）
     */
    explicit Client(IUdpTransport& transport, asio::any_io_executor exec);
    /**
     * @brief 主动连接服务器
     * @param conv 预先分配的会话 ID
     * @param server_ep 服务器的 UDP 地址
     * @return 建立好的会话对象
     */
    std::shared_ptr<KcpSession> connect(uint32_t conv, const asio::ip::udp::endpoint& server_ep);

    /**
     * @brief 更新所有会话状态
     * @param now_ms 当前时间点
     * @param timeout_sec 会话超时阈值（默认30秒）
     */
    using KcpEndpoint::update;

protected:
    /**
     * @brief 识别逻辑：直接解析包里的 conv
     */
    uint32_t selectConv(const asio::ip::udp::endpoint&, std::span<const uint8_t> data) override;

    // 客户端通常不需要在 onSession 里 co_spawn 协程，
    // 因为调用 connect 的地方通常就在协程里。
};