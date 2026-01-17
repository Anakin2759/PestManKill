#pragma once
#include "KcpEndpoint.h"
#include <asio.hpp>
#include <bit>
#include <cstring>
#include <utility>
#include "PeekConv.h"

class Server : public KcpEndpoint
{
public:
    // 构造函数：需要 IO 执行器（用于 KCP）和 线程池大小
    Server(IUdpTransport& transport, asio::any_io_executor io_exec, size_t thread_count);

    // 停止服务器，等待线程池结束
    void stop();

protected:
    /**
     * @brief 识别逻辑：直接解析包里的 conv
     */
    uint32_t selectConv(const asio::ip::udp::endpoint&, std::span<const uint8_t> data) override;

    /**
     * @brief 会话创建回调
     * @param conv 会话的 Conv ID
     * @param session 新创建的 KCP 会话
     */
    void onSession(uint32_t conv, std::shared_ptr<KcpSession> session) override;

private:
    /**
     * @brief 玩家业务“例程”：纯协程写法
     */
    static asio::awaitable<void> playerRoutine(uint32_t conv, std::shared_ptr<KcpSession> session);

private:
    asio::thread_pool m_pool;
};