#include "Server.h"

#include <asio.hpp>
#include <iostream>
#include <utility>

Server::Server(IUdpTransport& transport, asio::any_io_executor io_exec, size_t thread_count)
    : KcpEndpoint(std::move(io_exec), transport), m_pool(thread_count)
{
}

void Server::stop()
{
    m_pool.join();
}

uint32_t Server::selectConv([[maybe_unused]] const asio::ip::udp::endpoint& from, std::span<const uint8_t> data)
{
    return peekConv(data);
}

void Server::onSession(uint32_t conv, std::shared_ptr<KcpSession> session)
{
    // 1. 为每个玩家创建一个 Strand，保证该玩家的协程在线程池中是线程安全的
    auto player_executor = asio::make_strand(m_pool.get_executor());

    // 2. 启动玩家业务协程
    // 注意：这里将协程派发到线程池执行，而 KCP 的 input/update 依然在 IO 线程
    asio::co_spawn(player_executor, playerRoutine(conv, std::move(session)), asio::detached);

    std::cout << "Log: Player " << conv << " connected. Business logic moved to thread pool." << std::endl;
}

asio::awaitable<void> Server::playerRoutine(uint32_t conv, std::shared_ptr<KcpSession> session)
{
    try
    {
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