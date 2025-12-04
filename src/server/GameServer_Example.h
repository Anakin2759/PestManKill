/**
 * ************************************************************************
 *
 * @file GameServer_Example.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-04
 * @version 0.1
 * @brief 游戏服务器示例 - 演示多客户端通信
 *
 * 展示如何使用 NetWorkManager + ClientSessionManager 管理多个客户端:
 * 1. 客户端登录与注册
 * 2. 消息路由到正确的玩家实体
 * 3. 单播、广播消息
 * 4. 心跳检测与超时处理
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <memory>
#include "src/server/net/NetWorkManager.h"
#include "src/server/net/ClientSessionManager.h"
#include "src/server/context/GameContext.h"
#include "src/server/components/Player.h"
#include "src/server/components/Character.h"
#include "src/shared/common/Common.h"

// 简单的消息类型定义
enum class ServerMessageType : uint16_t
{
    LOGIN_REQUEST = 1,
    LOGIN_SUCCESS = 2,
    HEARTBEAT = 3,
    GAME_ACTION = 4,
    BROADCAST_EVENT = 5,
    LOGOUT = 6,
    KICKED = 7
};

/**
 * @brief 游戏服务器示例类
 */
class GameServerExample : public std::enable_shared_from_this<GameServerExample>
{
public:
    explicit GameServerExample(uint16_t port) : m_port(port)
    {
        // 初始化网络管理器
        m_networkManager = std::make_shared<NetWorkManager>(m_context.threadPool, m_context.logger);

        // 初始化会话管理器
        m_sessionManager = std::make_shared<ClientSessionManager>(m_context.logger);
    }

    void start()
    {
        m_context.logger->info("=== 游戏服务器启动 ===");

        // 设置网络消息处理器
        m_networkManager->setPacketHandler(
            [self = shared_from_this()](
                uint16_t type, const uint8_t* data, size_t size, const asio::ip::udp::endpoint& sender)
            { self->onPacketReceived(type, data, size, sender); });

        // 启动网络监听
        m_networkManager->start(m_port);
        m_context.logger->info("监听端口: {}", m_port);

        // 启动心跳检测定时器
        startHeartbeatTimer();

        m_context.logger->info("服务器就绪，等待客户端连接...");
    }

    void stop()
    {
        m_context.logger->info("服务器关闭中...");
        m_heartbeatTimer.reset();
        m_networkManager->stop();
    }

private:
    // ==================== 消息处理 ====================

    /**
     * @brief 收到网络数据包的回调
     */
    void onPacketReceived(uint16_t type, const uint8_t* data, size_t size, const asio::ip::udp::endpoint& sender)
    {
        auto msgType = static_cast<ServerMessageType>(type);
        const uint8_t* payload = data;
        size_t payloadSize = size;

        m_context.logger->debug("收到消息: type={}, size={}, from={}", type, size, sender.address().to_string());

        // 分发到具体处理函数
        switch (msgType)
        {
            case ServerMessageType::LOGIN_REQUEST:
                handleLoginRequest(payload, payloadSize, sender);
                break;

            case ServerMessageType::HEARTBEAT:
                handleHeartbeat(sender);
                break;

            case ServerMessageType::GAME_ACTION:
                handleGameAction(payload, payloadSize, sender);
                break;

            case ServerMessageType::LOGOUT:
                handleLogout(sender);
                break;

            default:
                m_context.logger->warn("未知消息类型: {} from {}", type, sender.address().to_string());
        }
    }

    /**
     * @brief 处理客户端登录请求
     */
    void handleLoginRequest(const uint8_t* data, size_t size, const asio::ip::udp::endpoint& sender)
    {
        if (size == 0)
        {
            m_context.logger->warn("登录请求无玩家名称");
            return;
        }

        // 解析玩家名称
        std::string playerName(reinterpret_cast<const char*>(data), size);
        m_context.logger->info("登录请求: 玩家={}, 地址={}", playerName, sender.address().to_string());

        // 检查人数限制
        if (m_sessionManager->getClientCount() >= ClientSessionManager::MAX_CLIENTS)
        {
            m_context.logger->warn("服务器已满，拒绝玩家 {} 登录", playerName);
            sendLoginFailed(sender, "服务器已满");
            return;
        }

        // 创建玩家实体
        auto playerEntity = createPlayerEntity(playerName);

        // 注册会话
        uint32_t clientId = m_sessionManager->registerClient(sender, playerEntity, playerName);

        // 发送登录成功响应
        sendLoginSuccess(sender, clientId, playerEntity);

        // 广播玩家加入事件给其他玩家
        broadcastPlayerJoined(playerName);

        m_context.logger->info(
            "玩家 {} 登录成功 (ClientID={}, Entity={})", playerName, clientId, static_cast<uint32_t>(playerEntity));
    }

    /**
     * @brief 处理心跳包
     */
    void handleHeartbeat(const asio::ip::udp::endpoint& sender)
    {
        m_sessionManager->updateHeartbeat(sender);
        m_context.logger->debug("更新心跳: {}", sender.address().to_string());
    }

    /**
     * @brief 处理游戏行为消息（示例：出牌、技能等）
     */
    void handleGameAction(const uint8_t* data, size_t size, const asio::ip::udp::endpoint& sender)
    {
        // 验证客户端身份
        auto playerEntity = m_sessionManager->getPlayerEntity(sender);
        if (!playerEntity)
        {
            m_context.logger->warn("未认证的客户端尝试发送游戏行为: {}", sender.address().to_string());
            return;
        }

        // 更新心跳（玩家活跃）
        m_sessionManager->updateHeartbeat(sender);

        // 获取玩家信息
        auto* playerInfo = m_context.registry.try_get<MetaPlayerInfo>(*playerEntity);
        if (!playerInfo)
        {
            m_context.logger->error("玩家实体 {} 缺少 MetaPlayerInfo 组件", static_cast<uint32_t>(*playerEntity));
            return;
        }

        // 解析游戏行为（这里简化为字符串）
        std::string action(reinterpret_cast<const char*>(data), size);

        m_context.logger->info("玩家 {} 执行行为: {}", playerInfo->playerName, action);

        // 这里可以触发 ECS 事件
        // m_context.dispatcher.trigger<events::PlayerAction>(*playerEntity, action);

        // 广播给其他玩家
        broadcastGameAction(playerInfo->playerName, action, *playerEntity);
    }

    /**
     * @brief 处理客户端登出
     */
    void handleLogout(const asio::ip::udp::endpoint& sender)
    {
        auto playerEntity = m_sessionManager->getPlayerEntity(sender);
        if (!playerEntity)
        {
            return;
        }

        auto* playerInfo = m_context.registry.try_get<MetaPlayerInfo>(*playerEntity);
        std::string playerName = playerInfo ? playerInfo->playerName : "Unknown";

        m_context.logger->info("玩家 {} 主动登出", playerName);

        // 断开会话
        m_sessionManager->disconnectClient(sender);

        // 销毁实体
        m_context.registry.destroy(*playerEntity);

        // 广播玩家离开
        broadcastPlayerLeft(playerName);
    }

    // ==================== 消息发送 ====================

    /**
     * @brief 发送登录成功响应
     */
    void sendLoginSuccess(const asio::ip::udp::endpoint& target, uint32_t clientId, entt::entity playerEntity)
    {
        // 构造响应: [clientId(4字节)][entityId(4字节)]
        std::vector<uint8_t> payload(sizeof(uint32_t) * 2);
        std::memcpy(payload.data(), &clientId, sizeof(uint32_t));
        uint32_t entityId = static_cast<uint32_t>(playerEntity);
        std::memcpy(payload.data() + sizeof(uint32_t), &entityId, sizeof(uint32_t));

        asio::co_spawn(
            m_context.threadPool.get_executor(),
            [self = shared_from_this(), target, payload]() -> asio::awaitable<void>
            {
                co_await self->m_networkManager->sendReliablePacket(
                    target, static_cast<uint16_t>(ServerMessageType::LOGIN_SUCCESS), payload);
            },
            asio::detached);
    }

    /**
     * @brief 发送登录失败响应
     */
    void sendLoginFailed(const asio::ip::udp::endpoint& target, const std::string& reason)
    {
        std::vector<uint8_t> payload(reason.begin(), reason.end());

        asio::co_spawn(
            m_context.threadPool.get_executor(),
            [self = shared_from_this(), target, payload]() -> asio::awaitable<void>
            {
                co_await self->m_networkManager->sendPacket(
                    target, static_cast<uint16_t>(ServerMessageType::KICKED), payload);
            },
            asio::detached);
    }

    /**
     * @brief 广播玩家加入事件
     */
    void broadcastPlayerJoined(const std::string& playerName)
    {
        std::string message = "玩家 " + playerName + " 加入了游戏";
        std::vector<uint8_t> payload(message.begin(), message.end());

        auto endpoints = m_sessionManager->getAllEndpoints();

        asio::co_spawn(
            m_context.threadPool.get_executor(),
            [self = shared_from_this(), endpoints, payload]() -> asio::awaitable<void>
            {
                for (const auto& endpoint : endpoints)
                {
                    co_await self->m_networkManager->sendPacket(
                        endpoint, static_cast<uint16_t>(ServerMessageType::BROADCAST_EVENT), payload);
                }
            },
            asio::detached);
    }

    /**
     * @brief 广播玩家离开事件
     */
    void broadcastPlayerLeft(const std::string& playerName)
    {
        std::string message = "玩家 " + playerName + " 离开了游戏";
        std::vector<uint8_t> payload(message.begin(), message.end());

        auto endpoints = m_sessionManager->getAllEndpoints();

        asio::co_spawn(
            m_context.threadPool.get_executor(),
            [self = shared_from_this(), endpoints, payload]() -> asio::awaitable<void>
            {
                for (const auto& endpoint : endpoints)
                {
                    co_await self->m_networkManager->sendPacket(
                        endpoint, static_cast<uint16_t>(ServerMessageType::BROADCAST_EVENT), payload);
                }
            },
            asio::detached);
    }

    /**
     * @brief 广播游戏行为（排除发送者）
     */
    void broadcastGameAction(const std::string& playerName, const std::string& action, entt::entity excludeEntity)
    {
        std::string message = playerName + " 执行了: " + action;
        std::vector<uint8_t> payload(message.begin(), message.end());

        auto allEndpoints = m_sessionManager->getAllEndpoints();
        auto excludeEndpoint = m_sessionManager->getEndpoint(excludeEntity);

        asio::co_spawn(
            m_context.threadPool.get_executor(),
            [self = shared_from_this(), allEndpoints, excludeEndpoint, payload]() -> asio::awaitable<void>
            {
                for (const auto& endpoint : allEndpoints)
                {
                    // 跳过发送者自己
                    if (excludeEndpoint && endpoint == *excludeEndpoint)
                    {
                        continue;
                    }

                    co_await self->m_networkManager->sendPacket(
                        endpoint, static_cast<uint16_t>(ServerMessageType::BROADCAST_EVENT), payload);
                }
            },
            asio::detached);
    }

    // ==================== 心跳检测 ====================

    /**
     * @brief 启动心跳检测定时器
     */
    void startHeartbeatTimer()
    {
        m_heartbeatTimer = std::make_shared<asio::steady_timer>(m_context.threadPool.get_executor());
        scheduleHeartbeatCheck();
    }

    /**
     * @brief 递归调度心跳检查
     */
    void scheduleHeartbeatCheck()
    {
        if (!m_heartbeatTimer)
        {
            return;
        }

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        m_heartbeatTimer->expires_after(std::chrono::seconds(10));

        m_heartbeatTimer->async_wait(
            [self = shared_from_this()](const asio::error_code& error)
            {
                if (!error)
                {
                    self->checkClientTimeouts();
                    self->scheduleHeartbeatCheck(); // 继续下一次检查
                }
            });
    }

    /**
     * @brief 检查超时客户端
     */
    void checkClientTimeouts()
    {
        auto timedOutPlayers = m_sessionManager->checkTimeouts();

        if (timedOutPlayers.empty())
        {
            return;
        }

        m_context.logger->info("检测到 {} 个超时客户端", timedOutPlayers.size());

        for (auto playerEntity : timedOutPlayers)
        {
            // 获取玩家信息
            auto* playerInfo = m_context.registry.try_get<MetaPlayerInfo>(playerEntity);
            std::string playerName = playerInfo ? playerInfo->playerName : "Unknown";

            m_context.logger->warn(
                "玩家 {} (Entity={}) 心跳超时，已断开", playerName, static_cast<uint32_t>(playerEntity));

            // 广播玩家掉线
            broadcastPlayerLeft(playerName);

            // 触发断线事件（可选）
            // m_context.dispatcher.trigger<events::PlayerDisconnected>(playerEntity);

            // 销毁实体
            m_context.registry.destroy(playerEntity);
        }
    }

    // ==================== 辅助方法 ====================

    /**
     * @brief 创建玩家实体并初始化组件
     */
    entt::entity createPlayerEntity(const std::string& playerName)
    {
        auto entity = m_context.registry.create();

        // 基础玩家信息
        m_context.registry.emplace<MetaPlayerInfo>(entity, playerName, DEFAULT_PLAYER_ID);

        // 角色属性（示例：默认 4 血）
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
        m_context.registry.emplace<Attributes>(entity, 4, 4, 1);

        // 手牌（空）
        m_context.registry.emplace<HandCards>(entity);

        // 装备（空）
        m_context.registry.emplace<Equipments>(entity);

        return entity;
    }

    // ==================== 成员变量 ====================

    GameContext m_context;
    std::shared_ptr<NetWorkManager> m_networkManager;
    std::shared_ptr<ClientSessionManager> m_sessionManager;
    std::shared_ptr<asio::steady_timer> m_heartbeatTimer;
    uint16_t m_port;
};
