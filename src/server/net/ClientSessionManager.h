/**
 * ************************************************************************
 *
 * @file ClientSessionManager.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-04
 * @version 0.1
 * @brief 客户端会话管理器
 * 负责管理多个客户端连接，将 UDP endpoint 映射到游戏实体
 * - endpoint -> entity 映射
 * - 客户端生命周期管理
 * - 广播消息到所有/部分客户端
 * - 心跳检测与超时处理
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <asio.hpp>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <optional>
#include <entt/entt.hpp>
#include <spdlog/spdlog.h>
#include "absl/container/flat_hash_map.h"

/**
 * @brief 客户端会话信息
 */
struct ClientSession
{
    asio::ip::udp::endpoint endpoint;                       // UDP 地址
    entt::entity playerEntity = entt::null;                 // 对应的玩家实体
    std::chrono::steady_clock::time_point lastHeartbeat;    // 最后心跳时间
    uint32_t clientId;                                      // 唯一客户端ID
    std::string playerName;                                 // 玩家名称
    bool isAuthenticated = false;                           // 是否已认证
    bool isDisconnected = false;                            // 是否已掉线（仅标记，不移除）
    std::chrono::steady_clock::time_point disconnectedTime; // 掉线时间
};

/**
 * @brief 客户端会话管理器
 *
 * 使用方式:
 * 1. 收到新客户端消息时，调用 registerClient() 或 getOrCreateSession()
 * 2. 通过 endpoint 查询对应的玩家实体: getPlayerEntity(endpoint)
 * 3. 通过玩家实体查询 endpoint: getEndpoint(entity)
 * 4. 广播消息: broadcastToAll() 或 broadcastToPlayers()
 * 5. 定期调用 checkTimeouts() 清理超时客户端
 */
class ClientSessionManager
{
public:
    static constexpr int MAX_CLIENTS = 8;
    static constexpr std::chrono::seconds HEARTBEAT_TIMEOUT{30};

    explicit ClientSessionManager(std::shared_ptr<spdlog::logger> logger) : m_logger(std::move(logger)) {}

    /**
     * @brief 注册新客户端
     * @param endpoint 客户端地址
     * @param playerEntity 对应的玩家实体
     * @param playerName 玩家名称
     * @return 客户端ID，如果已存在则返回现有ID
     */
    uint32_t registerClient(const asio::ip::udp::endpoint& endpoint,
                            entt::entity playerEntity,
                            const std::string& playerName)
    {
        std::scoped_lock lock(m_mutex);

        // 检查是否已存在
        if (auto iter = m_endpointToSession.find(endpoint); iter != m_endpointToSession.end())
        {
            auto& session = m_sessions[iter->second];
            session.lastHeartbeat = std::chrono::steady_clock::now();
            session.playerEntity = playerEntity;
            session.playerName = playerName;
            session.isAuthenticated = true;
            std::string addrStr = endpoint.address().to_string();
            m_logger->info("客户端 {} 重新认证，玩家: {}", addrStr, playerName);
            return session.clientId;
        }

        // 创建新会话
        uint32_t clientId = m_nextClientId++;
        ClientSession session{.endpoint = endpoint,
                              .playerEntity = playerEntity,
                              .lastHeartbeat = std::chrono::steady_clock::now(),
                              .clientId = clientId,
                              .playerName = playerName,
                              .isAuthenticated = true};

        m_sessions[clientId] = session;
        m_endpointToSession[endpoint] = clientId;
        m_entityToClientId[playerEntity] = clientId;

        m_logger->info("注册客户端 {}, ID: {}, 玩家: {}, Entity: {}",
                       endpoint.address().to_string(),
                       clientId,
                       playerName,
                       static_cast<uint32_t>(playerEntity));

        return clientId;
    }

    /**
     * @brief 更新客户端心跳时间
     */
    void updateHeartbeat(const asio::ip::udp::endpoint& endpoint)
    {
        std::scoped_lock lock(m_mutex);
        if (auto iter = m_endpointToSession.find(endpoint); iter != m_endpointToSession.end())
        {
            m_sessions[iter->second].lastHeartbeat = std::chrono::steady_clock::now();
        }
    }

    /**
     * @brief 通过 endpoint 获取玩家实体
     * @param allowDisconnected 是否允许返回已掉线的玩家实体（默认 false）
     */
    std::optional<entt::entity> getPlayerEntity(const asio::ip::udp::endpoint& endpoint,
                                                bool allowDisconnected = false) const
    {
        std::scoped_lock lock(m_mutex);
        if (auto iter = m_endpointToSession.find(endpoint); iter != m_endpointToSession.end())
        {
            const auto& session = m_sessions.at(iter->second);
            if (session.isAuthenticated && session.playerEntity != entt::null)
            {
                // 如果不允许掉线且客户端已掉线，返回空
                if (!allowDisconnected && session.isDisconnected)
                {
                    return std::nullopt;
                }
                return session.playerEntity;
            }
        }
        return std::nullopt;
    }

    /**
     * @brief 检查客户端是否掉线
     */
    bool isClientDisconnected(const asio::ip::udp::endpoint& endpoint) const
    {
        std::scoped_lock lock(m_mutex);
        if (auto iter = m_endpointToSession.find(endpoint); iter != m_endpointToSession.end())
        {
            return m_sessions.at(iter->second).isDisconnected;
        }
        return false;
    }

    /**
     * @brief 检查玩家实体是否掉线
     */
    bool isPlayerDisconnected(entt::entity playerEntity) const
    {
        std::scoped_lock lock(m_mutex);
        if (auto iter = m_entityToClientId.find(playerEntity); iter != m_entityToClientId.end())
        {
            return m_sessions.at(iter->second).isDisconnected;
        }
        return false;
    }

    /**
     * @brief 客户端重新连接（清除掉线标记）
     * @return 是否成功重连
     */
    bool reconnectClient(const asio::ip::udp::endpoint& endpoint)
    {
        std::scoped_lock lock(m_mutex);
        if (auto iter = m_endpointToSession.find(endpoint); iter != m_endpointToSession.end())
        {
            auto& session = m_sessions[iter->second];
            if (session.isDisconnected)
            {
                session.isDisconnected = false;
                session.lastHeartbeat = std::chrono::steady_clock::now();
                std::string addrStr = endpoint.address().to_string();
                m_logger->info("客户端 {} 重新连接，玩家: {}", addrStr, session.playerName);
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 通过玩家实体获取 endpoint
     */
    std::optional<asio::ip::udp::endpoint> getEndpoint(entt::entity playerEntity) const
    {
        std::scoped_lock lock(m_mutex);
        if (auto iter = m_entityToClientId.find(playerEntity); iter != m_entityToClientId.end())
        {
            return m_sessions.at(iter->second).endpoint;
        }
        return std::nullopt;
    }

    /**
     * @brief 获取所有已认证客户端的 endpoint 列表
     * @param excludeDisconnected 是否排除已掉线的客户端（默认 true）
     */
    std::vector<asio::ip::udp::endpoint> getAllEndpoints(bool excludeDisconnected = true) const
    {
        std::scoped_lock lock(m_mutex);
        std::vector<asio::ip::udp::endpoint> endpoints;
        endpoints.reserve(m_sessions.size());

        for (const auto& [id, session] : m_sessions)
        {
            if (session.isAuthenticated)
            {
                // 根据参数决定是否排除掉线客户端
                if (excludeDisconnected && session.isDisconnected)
                {
                    continue;
                }
                endpoints.push_back(session.endpoint);
            }
        }
        return endpoints;
    }

    /**
     * @brief 获取所有掉线的客户端 endpoint 列表
     */
    std::vector<asio::ip::udp::endpoint> getDisconnectedEndpoints() const
    {
        std::scoped_lock lock(m_mutex);
        std::vector<asio::ip::udp::endpoint> endpoints;

        for (const auto& [id, session] : m_sessions)
        {
            if (session.isDisconnected)
            {
                endpoints.push_back(session.endpoint);
            }
        }
        return endpoints;
    }

    /**
     * @brief 获取指定玩家实体列表的 endpoint
     */
    std::vector<asio::ip::udp::endpoint> getEndpoints(const std::vector<entt::entity>& players) const
    {
        std::scoped_lock lock(m_mutex);
        std::vector<asio::ip::udp::endpoint> endpoints;
        endpoints.reserve(players.size());

        for (auto entity : players)
        {
            if (auto iter = m_entityToClientId.find(entity); iter != m_entityToClientId.end())
            {
                endpoints.push_back(m_sessions.at(iter->second).endpoint);
            }
        }
        return endpoints;
    }

    /**
     * @brief 断开客户端连接
     */
    void disconnectClient(const asio::ip::udp::endpoint& endpoint)
    {
        std::scoped_lock lock(m_mutex);
        if (auto iter = m_endpointToSession.find(endpoint); iter != m_endpointToSession.end())
        {
            uint32_t clientId = iter->second;
            const auto& session = m_sessions[clientId];

            std::string addrStr = endpoint.address().to_string();
            m_logger->info("断开客户端 {}, 玩家: {}", addrStr, session.playerName);

            m_entityToClientId.erase(session.playerEntity);
            m_sessions.erase(clientId);
            m_endpointToSession.erase(iter);
        }
    }

    /**
     * @brief 检查超时的客户端并标记为掉线（不移除会话）
     * @return 新掉线的玩家实体列表
     */
    std::vector<entt::entity> checkTimeouts()
    {
        std::scoped_lock lock(m_mutex);
        auto now = std::chrono::steady_clock::now();
        std::vector<entt::entity> newlyDisconnected;

        for (auto& [clientId, session] : m_sessions)
        {
            // 跳过已标记掉线的
            if (session.isDisconnected)
            {
                continue;
            }

            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - session.lastHeartbeat);
            if (elapsed > HEARTBEAT_TIMEOUT)
            {
                m_logger->warn("客户端 {} 心跳超时 ({}s), 玩家: {} - 标记为掉线",
                               session.endpoint.address().to_string(),
                               elapsed.count(),
                               session.playerName);

                // 标记为掉线，但不移除会话
                session.isDisconnected = true;
                session.disconnectedTime = now;
                newlyDisconnected.push_back(session.playerEntity);
            }
        }

        return newlyDisconnected;
    }

    /**
     * @brief 强制移除客户端会话（游戏结束时调用）
     * @param endpoint 客户端地址
     */
    void removeClient(const asio::ip::udp::endpoint& endpoint)
    {
        std::scoped_lock lock(m_mutex);
        if (auto iter = m_endpointToSession.find(endpoint); iter != m_endpointToSession.end())
        {
            uint32_t clientId = iter->second;
            const auto& session = m_sessions[clientId];

            std::string addrStr = endpoint.address().to_string();
            m_logger->info("移除客户端 {}, 玩家: {}", addrStr, session.playerName);

            m_entityToClientId.erase(session.playerEntity);
            m_sessions.erase(clientId);
            m_endpointToSession.erase(iter);
        }
    }

    /**
     * @brief 清理所有会话（游戏结束时调用）
     */
    void clearAllSessions()
    {
        std::scoped_lock lock(m_mutex);
        m_logger->info("清理所有客户端会话 (共 {} 个)", m_sessions.size());

        m_sessions.clear();
        m_endpointToSession.clear();
        m_entityToClientId.clear();
    }

    /**
     * @brief 获取当前在线客户端数量
     */
    size_t getClientCount() const
    {
        std::scoped_lock lock(m_mutex);
        return m_sessions.size();
    }

    /**
     * @brief 获取客户端会话信息
     */
    std::optional<ClientSession> getSession(const asio::ip::udp::endpoint& endpoint) const
    {
        std::scoped_lock lock(m_mutex);
        if (auto iter = m_endpointToSession.find(endpoint); iter != m_endpointToSession.end())
        {
            return m_sessions.at(iter->second);
        }
        return std::nullopt;
    }

private:
    mutable std::mutex m_mutex;
    std::shared_ptr<spdlog::logger> m_logger;

    // 客户端会话存储
    absl::flat_hash_map<uint32_t, ClientSession> m_sessions;

    // 快速查找映射
    absl::flat_hash_map<asio::ip::udp::endpoint, uint32_t> m_endpointToSession;
    absl::flat_hash_map<entt::entity, uint32_t> m_entityToClientId;

    std::atomic<uint32_t> m_nextClientId{1};
};

/**
 * @brief 为 asio::ip::udp::endpoint 提供 hash 支持
 */
namespace std
{
template <>
struct hash<asio::ip::udp::endpoint>
{
    std::size_t operator()(const asio::ip::udp::endpoint& endpoint) const
    {
        std::size_t h1 = std::hash<std::string>{}(endpoint.address().to_string());
        std::size_t h2 = std::hash<unsigned short>{}(endpoint.port());
        return h1 ^ (h2 << 1U);
    }
};
} // namespace std
