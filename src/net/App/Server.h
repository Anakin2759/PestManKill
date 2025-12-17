/**
 * ************************************************************************
 *
 * @file KcpServer.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-17
 * @version 0.1
 * @brief KCP 服务器端点定义
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "KcpEndpoint.h"
#include <cstring>
#include <bit>

#include <bit>
#include <endian.h>

inline uint32_t peekConv(std::span<const uint8_t> data)
{
    if (data.size() < 4) [[unlikely]]
        return 0;

    uint32_t conv;
    std::memcpy(&conv, data.data(), sizeof(uint32_t));

    // 如果当前原生机器是 大端序，则需要翻转字节以匹配 KCP 的小端序
    if constexpr (std::endian::native == std::endian::big)
    {
        return std::byteswap(conv); // C++23 提供的标准字节交换
    }
    return conv;
}

class Server final : public KcpEndpoint
{
public:
    explicit Server(IUdpTransport& transport) : KcpEndpoint(transport) {}

protected:
    uint32_t selectConv(const asio::ip::udp::endpoint&, std::span<const uint8_t> data) override
    {
        return peekConv(data);
    }
    /**
     * @brief 会话创建回调
     * @param conv 会话 ID
     * @param session 新创建的 KCP 会话对象
     */
    void onSession(uint32_t conv, std::shared_ptr<KcpSession> session) override
    {
        // 1. 业务逻辑：创建一个玩家对象
        auto newPlayer = std::make_shared<Player>(conv, session);

        // 2. 绑定回调：当 KCP 收到应用层数据时，通知玩家对象
        session->setDataCallback([newPlayer](std::span<const uint8_t> data) { newPlayer->handleMessage(data); });

        // 3. 保存到你的玩家管理器中
        m_players[conv] = newPlayer;

        std::cout << "Log: New player joined. Conv ID: " << conv << std::endl;
    }

    /**
     * @brief 会话关闭回调
     * @param conv 会话 ID
     */
    void onSessionClosed(uint32_t conv) override
    {
        m_players.erase(conv);
        std::cout << "Log: Player left. Conv ID: " << conv << std::endl;
    }
};