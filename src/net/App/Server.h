#pragma once
#include "KcpEndpoint.h"
#include <asio.hpp>
#include <iostream>

class Server final : public KcpEndpoint
{
public:
    // 构造函数：需要 IO 执行器（用于 KCP）和 线程池大小
    Server(IUdpTransport& transport, asio::any_io_executor io_exec, size_t thread_count)
        : KcpEndpoint(transport, std::move(io_exec)), m_pool(thread_count)
    {
    }

    // 停止服务器，等待线程池结束
    void stop() { m_pool.join(); }

protected:
    uint32_t selectConv(const asio::ip::udp::endpoint&, std::span<const uint8_t> data) override
    {
        return peekConv(data);
    }

    /**
     * @brief 核心调整：不再写回调，直接派生协程
     */
    void onSession(uint32_t conv, std::shared_ptr<KcpSession> session) override
    {
        // 1. 为每个玩家创建一个 Strand，保证该玩家的协程在线程池中是线程安全的
        auto player_executor = asio::make_strand(m_pool.get_executor());

        // 2. 启动玩家业务协程
        // 注意：这里将协程派发到线程池执行，而 KCP 的 input/update 依然在 IO 线程
        asio::co_spawn(player_executor, playerRoutine(conv, std::move(session)), asio::detached);

        std::cout << "Log: Player " << conv << " connected. Business logic moved to thread pool." << std::endl;
    }

private:
    /**
     * @brief 玩家业务“例程”：纯协程写法
     */
    static asio::awaitable<void> playerRoutine(uint32_t conv, std::shared_ptr<KcpSession> session)
    {
        try
        {
            // 业务逻辑可以在此直接写，例如：
            // auto login_data = co_await session->recv();
            // if (!verify(login_data)) co_return;

            while (true)
            {
                // 挂起协程，等待 KCP 层通过 channel 传来的数据
                auto result = co_await session->recv();

                if (!result)
                {
                    break; // 连接断开或超时
                }

                const auto& msg = *result;

                // 处理业务逻辑（此时运行在线程池中）
                // ... handle(msg) ...

                // 发送回执
                session->send(msg);
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Player " << conv << " exception: " << e.what() << std::endl;
        }

        std::cout << "Log: Player " << conv << " disconnected." << std::endl;
    }

private:
    asio::thread_pool m_pool;
};