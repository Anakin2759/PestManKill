/**
 * ************************************************************************
 *
 * @file GameClient.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-04
 * @version 0.1
 * @brief 游戏客户端网络管理器
 *
 * 职责：
 * - 封装网络通信细节
 * - 提供高层游戏接口（登录、登出、发送游戏消息）
 * - 一个客户端是房间所有者 ，其他客户端是普通玩家
 * - 一个客户端对应一个玩家，只能在一个房间内
 * - 每一个ip只能有一个房间
 * - connect成功启动异步事件循环处理各类请求
 * - 处理服务器响应并触发客户端事件
 * - 心跳管理 客户端在连接服务器时每隔固定时间发送心跳包
 * - 可靠消息发送（基于 NetWorkClient 的 ACK 机制）
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <memory>
#include <string>
#include <functional>
#include <chrono>
#include <atomic>
#include "src/client/net/NetWorkClient.h"
#include "src/shared/messages/request/UseCardRequest.h"
#include "src/shared/messages/request/DiscardCardRequest.h"
#include "src/shared/common/Common.h"
#include "src/client/utils/Logger.h"
#include "src/client/utils/ThreadPool.h"
#include <nlohmann/json.hpp>
#include <entt/entt.hpp>
#include "net/IServerMessageHandler.h"

/**
 * @brief 客户端连接状态
 */
enum class ClientState : uint8_t
{
    DISCONNECTED,  // 未连接
    CONNECTING,    // 连接中
    CONNECTED,     // 已连接（但未登录）
    AUTHENTICATED, // 已认证（登录成功）
    IN_GAME        // 游戏中
};
/**
 * @brief 游戏客户端主类
 */
class GameClient : public std::enable_shared_from_this<GameClient>
{
public:
    GameClient() : m_networkClient(std::make_shared<NetWorkClient>()) {}

    ~GameClient() { disconnect(); }

    GameClient(const GameClient&) = delete;
    GameClient& operator=(const GameClient&) = delete;
    GameClient(GameClient&& other) = delete;
    GameClient& operator=(GameClient&& other) = delete;

    // ==================== 连接管理 ====================

    /**
     * @brief 连接到服务器（异步）
     * @param host 服务器地址
     * @param port 服务器端口
     */
    asio::awaitable<void> connect(const std::string& host, uint16_t port)
    {
        if (m_state != ClientState::DISCONNECTED)
        {
            utils::LOG_WARN("Already connected or connecting, ignoring duplicate connection request");
            co_return;
        }

        m_state = ClientState::CONNECTING;
        utils::LOG_INFO("Connecting to server: {}:{}", host, port);

        // 设置消息处理器
        m_networkClient->setPacketHandler(
            [self = shared_from_this()](
                uint16_t type, const uint8_t* data, size_t size, const asio::ip::udp::endpoint& sender)
            { self->onPacketReceived(type, data, size, sender); });

        // 启动网络层
        m_networkClient->start();
        m_networkClient->connect(host, port);

        // 等待网络层初始化完成
        auto timer = std::make_shared<asio::steady_timer>(utils::ThreadPool::getInstance().get_executor(),
                                                          std::chrono::milliseconds(100));
        co_await timer->async_wait(asio::use_awaitable);

        m_state = ClientState::CONNECTED;

        if (m_messageHandler)
        {
            m_messageHandler->onConnected();
        }

        utils::LOG_INFO("Connection successful");
    }

    /**
     * @brief 断开连接
     */
    void disconnect() noexcept
    {
        try
        {
            if (m_state == ClientState::DISCONNECTED)
            {
                return;
            }

            utils::LOG_INFO("Disconnecting from server");

            // Stop heartbeat
            stopHeartbeat();

            // 如果已登录，先发送登出消息
            if (m_state == ClientState::AUTHENTICATED || m_state == ClientState::IN_GAME)
            {
                sendLogout();
            }

            if (m_networkClient)
            {
                m_networkClient->stop();
            }

            m_state = ClientState::DISCONNECTED;

            if (m_messageHandler)
            {
                m_messageHandler->onDisconnected();
            }
        }
        catch (const std::exception& e)
        {
            utils::LOG_ERROR("Exception during disconnect: {}", e.what());
        }
        catch (...)
        {
            utils::LOG_ERROR("Unknown exception during disconnect");
        }
    }

    /**
     * @brief 设置消息处理器
     */
    void setMessageHandler(entt::poly<IServerMessageHandler> handler) { m_messageHandler = std::move(handler); }

    /**
     * @brief 获取当前状态
     */
    ClientState getState() const { return m_state; }

    /**
     * @brief 获取客户端 ID
     */
    uint32_t getClientId() const { return m_clientId; }

    /**
     * @brief 获取玩家实体 ID
     */
    uint32_t getPlayerEntity() const { return m_playerEntity; }

    // ==================== 登录/登出 ====================

    /**
     * @brief 登录到服务器
     * @param playerName 玩家名称
     */
    asio::awaitable<void> login(const std::string& playerName)
    {
        if (m_state != ClientState::CONNECTED)
        {
            utils::LOG_ERROR("Cannot login: not connected to server");
            co_return;
        }

        utils::LOG_INFO("Sending login request: {}", playerName);
        m_playerName = playerName;

        // 构造登录消息（简单的字符串）
        std::vector<uint8_t> payload(playerName.begin(), playerName.end());

        // 发送可靠消息
        bool success = co_await m_networkClient->sendReliablePacket(
            static_cast<uint16_t>(RequestType::LOGIN), payload, 3, std::chrono::milliseconds(2000));

        if (!success)
        {
            utils::LOG_ERROR("Failed to send login request");
            if (m_messageHandler)
            {
                m_messageHandler->onLoginFailed("Network error");
            }
        }
    }

    /**
     * @brief 登出
     */
    void sendLogout()
    {
        if (m_state != ClientState::AUTHENTICATED && m_state != ClientState::IN_GAME)
        {
            return;
        }

        utils::LOG_INFO("Sending logout message");

        std::vector<uint8_t> emptyPayload;

        asio::co_spawn(
            utils::ThreadPool::getInstance().get_executor(),
            [self = shared_from_this(), emptyPayload]() -> asio::awaitable<void>
            { co_await self->m_networkClient->sendPacket(static_cast<uint16_t>(RequestType::LOGOUT), emptyPayload); },
            asio::detached);
    }

    // ==================== 游戏消息发送 ====================

    /**
     * @brief 发送使用卡牌消息
     */
    asio::awaitable<bool> sendUseCard(uint32_t card, const std::vector<uint32_t>& targets)
    {
        if (m_state != ClientState::IN_GAME)
        {
            utils::LOG_ERROR("Cannot send use card message: not in game");
            co_return false;
        }

        UseCardRequest msg{.player = m_playerEntity, .card = card, .targets = targets};

        std::string jsonStr = msg.toJson().dump();
        std::vector<uint8_t> payload(jsonStr.begin(), jsonStr.end());

        // 将 targets 转换为字符串格式以便日志输出
        std::string targetsStr = "[";
        for (size_t i = 0; i < targets.size(); ++i)
        {
            targetsStr += std::to_string(targets[i]);
            if (i < targets.size() - 1) targetsStr += ",";
        }
        targetsStr += "]";
        utils::LOG_INFO("Sending use card: card={}, targets={}", card, targetsStr);

        bool success =
            co_await m_networkClient->sendReliablePacket(static_cast<uint16_t>(RequestType::USE_CARD), payload);

        if (!success)
        {
            utils::LOG_ERROR("Failed to send use card message");
        }

        co_return success;
    }

    /**
     * @brief 发送弃牌消息
     */
    asio::awaitable<bool> sendDiscardCard(const std::vector<uint32_t>& cards)
    {
        if (m_state != ClientState::IN_GAME)
        {
            utils::LOG_ERROR("Cannot send discard card message: not in game");
            co_return false;
        }

        DiscardCardRequest msg{.player = m_playerEntity, .cardIndexs = cards};

        std::string jsonStr = msg.toJson().dump();
        std::vector<uint8_t> payload(jsonStr.begin(), jsonStr.end());

        // 将 cards 转换为字符串格式以便日志输出
        std::string cardsStr = "[";
        for (size_t i = 0; i < cards.size(); ++i)
        {
            cardsStr += std::to_string(cards[i]);
            if (i < cards.size() - 1) cardsStr += ",";
        }
        cardsStr += "]";
        utils::LOG_INFO("Sending discard cards: cards={}", cardsStr);

        bool success =
            co_await m_networkClient->sendReliablePacket(static_cast<uint16_t>(RequestType::DISCARD_CARD), payload);

        if (!success)
        {
            utils::LOG_ERROR("Failed to send discard card message");
        }

        co_return success;
    }

    /**
     * @brief 发送结束回合消息
     */
    asio::awaitable<bool> sendEndTurn()
    {
        if (m_state != ClientState::IN_GAME)
        {
            utils::LOG_ERROR("Cannot send end turn message: not in game");
            co_return false;
        }

        utils::LOG_INFO("Sending end turn");

        std::vector<uint8_t> emptyPayload;

        bool success =
            co_await m_networkClient->sendReliablePacket(static_cast<uint16_t>(RequestType::END_TURN), emptyPayload);

        co_return success;
    }

    /**
     * @brief 发送聊天消息
     */
    asio::awaitable<void> sendChatMessage(const std::string& message)
    {
        std::vector<uint8_t> payload(message.begin(), message.end());

        co_await m_networkClient->sendPacket(static_cast<uint16_t>(RequestType::CHAT_MESSAGE), payload);
    }

    // ==================== 心跳管理 ====================

    /**
     * @brief 启动心跳
     */
    void startHeartbeat(std::chrono::seconds interval = std::chrono::seconds(5))
    {
        if (m_heartbeatRunning.exchange(true))
        {
            return; // 已在运行
        }

        m_heartbeatInterval = interval;
        scheduleHeartbeat();

        utils::LOG_INFO("Heartbeat started, interval: {}s", interval.count());
    }

    /**
     * @brief 停止心跳
     */
    void stopHeartbeat() noexcept
    {
        try
        {
            m_heartbeatRunning.store(false);
            if (m_heartbeatTimer)
            {
                asio::error_code ec;
                m_heartbeatTimer->cancel();
                m_heartbeatTimer.reset();
            }
        }
        catch (...)
        {
            // 忽略异常，确保 noexcept
        }
    }

private:
    // ==================== 消息接收处理 ====================

    /**
     * @brief 收到网络数据包的回调
     */
    void onPacketReceived(uint16_t msgType,
                          const uint8_t* data,
                          size_t size,
                          [[maybe_unused]] const asio::ip::udp::endpoint& sender)
    {
        utils::LOG_DEBUG("Received message: type={}, size={}", msgType, size);

        // 目前客户端侧仍然按旧协议解析：
        // 服务器用 LOGIN 作为登录响应、HEARTBEAT 作为心跳回显、
        // GAME_STATE / USE_CARD / BROADCAST_EVENT / ERROR_MESSAGE / CHAT_MESSAGE 保持不变。
        auto legacyType = static_cast<MessageType>(msgType);

        switch (legacyType)
        {
            case MessageType::LOGIN:
                handleLoginResponse(data, size);
                break;

            case MessageType::HEARTBEAT:
                // 心跳响应：服务器确认收到心跳，连接正常
                utils::LOG_DEBUG(
                    "♥ Heartbeat acknowledged by server ({}:{})", sender.address().to_string(), sender.port());
                break;

            case MessageType::GAME_STATE:
                handleGameStateUpdate(data, size);
                break;

            case MessageType::USE_CARD:
                handleUseCardResponse(data, size);
                break;

            case MessageType::BROADCAST_EVENT:
                handleBroadcastEvent(data, size);
                break;

            case MessageType::ERROR_MESSAGE:
                handleErrorMessage(data, size);
                break;

            case MessageType::CHAT_MESSAGE:
                handleChatMessage(data, size);
                break;

            default:
                utils::LOG_WARN("Unhandled message type: {}", msgType);
        }
    }

    /**
     * @brief 处理登录响应
     */
    void handleLoginResponse(const uint8_t* data, size_t size)
    {
        try
        {
            std::string jsonStr(reinterpret_cast<const char*>(data), size);
            nlohmann::json json = nlohmann::json::parse(jsonStr);

            m_clientId = json.at("clientId").get<uint32_t>();
            m_playerEntity = json.at("entityId").get<uint32_t>();

            utils::LOG_INFO("Login successful: clientId={}, playerEntity={}", m_clientId, m_playerEntity);

            m_state = ClientState::AUTHENTICATED;

            if (m_messageHandler)
            {
                m_messageHandler->onLoginSuccess(m_clientId, m_playerEntity);
            }

            // 启动心跳
            startHeartbeat();
        }
        catch (const std::exception& e)
        {
            utils::LOG_ERROR("Failed to parse login response: {}", e.what());
            if (m_messageHandler)
            {
                m_messageHandler->onLoginFailed("Invalid server response format");
            }
        }
    }

    /**
     * @brief 处理游戏状态更新（JSON 数据）
     */
    void handleGameStateUpdate(const uint8_t* data, size_t size)
    {
        try
        {
            std::string jsonStr(reinterpret_cast<const char*>(data), size);
            nlohmann::json json = nlohmann::json::parse(jsonStr);

            utils::LOG_DEBUG("Received game state update");

            if (m_state == ClientState::AUTHENTICATED)
            {
                m_state = ClientState::IN_GAME;

                if (m_messageHandler)
                {
                    m_messageHandler->onGameStart();
                }
            }

            if (m_messageHandler)
            {
                m_messageHandler->onGameStateUpdate(json);
            }
        }
        catch (const std::exception& e)
        {
            utils::LOG_ERROR("Failed to parse game state: {}", e.what());
        }
    }

    /**
     * @brief 处理使用卡牌响应（JSON 数据）
     */
    void handleUseCardResponse(const uint8_t* data, size_t size)
    {
        try
        {
            std::string jsonStr(reinterpret_cast<const char*>(data), size);
            nlohmann::json json = nlohmann::json::parse(jsonStr);

            bool success = json.value("success", false);
            std::string message = json.value("message", "");

            utils::LOG_INFO("Use card response: success={}, message={}", success, message);

            if (m_messageHandler)
            {
                m_messageHandler->onUseCardResponse(success, message);
            }
        }
        catch (const std::exception& e)
        {
            utils::LOG_ERROR("Failed to parse use card response: {}", e.what());
        }
    }

    /**
     * @brief 处理广播事件（文本数据）
     */
    void handleBroadcastEvent(const uint8_t* data, size_t size)
    {
        std::string message(reinterpret_cast<const char*>(data), size);
        utils::LOG_INFO("Broadcast event: {}", message);

        if (m_messageHandler)
        {
            // 解析特定事件
            if (message.find("加入了游戏") != std::string::npos)
            {
                size_t pos = message.find(" ");
                if (pos != std::string::npos)
                {
                    std::string playerName = message.substr(3, pos - 3); // "玩家 XXX"
                    m_messageHandler->onPlayerJoined(playerName);
                }
            }
            else if (message.find("离开了游戏") != std::string::npos)
            {
                size_t pos = message.find(" ");
                if (pos != std::string::npos)
                {
                    std::string playerName = message.substr(3, pos - 3);
                    m_messageHandler->onPlayerLeft(playerName);
                }
            }
            else if (message.find("掉线") != std::string::npos)
            {
                size_t pos = message.find(" ");
                if (pos != std::string::npos)
                {
                    std::string playerName = message.substr(3, pos - 3);
                    m_messageHandler->onPlayerDisconnected(playerName);
                }
            }
            else if (message.find("重新连接") != std::string::npos)
            {
                size_t pos = message.find(" ");
                if (pos != std::string::npos)
                {
                    std::string playerName = message.substr(3, pos - 3);
                    m_messageHandler->onPlayerReconnected(playerName);
                }
            }

            m_messageHandler->onBroadcastEvent(message);
        }
    }

    /**
     * @brief 处理错误消息（文本数据）
     */
    void handleErrorMessage(const uint8_t* data, size_t size)
    {
        std::string errorMessage(reinterpret_cast<const char*>(data), size);
        utils::LOG_ERROR("Server error: {}", errorMessage);

        if (m_messageHandler)
        {
            m_messageHandler->onError(errorMessage);
        }
    }

    /**
     * @brief 处理聊天消息（文本数据）
     */
    void handleChatMessage(const uint8_t* data, size_t size)
    {
        std::string message(reinterpret_cast<const char*>(data), size);
        utils::LOG_INFO("Chat message: {}", message);

        if (m_messageHandler)
        {
            m_messageHandler->onBroadcastEvent(message);
        }
    }

    // ==================== 心跳实现 ====================

    /**
     * @brief 调度下一次心跳
     */
    void scheduleHeartbeat()
    {
        if (!m_heartbeatRunning.load())
        {
            return;
        }

        m_heartbeatTimer = std::make_shared<asio::steady_timer>(utils::ThreadPool::getInstance().get_executor());
        m_heartbeatTimer->expires_after(m_heartbeatInterval);

        m_heartbeatTimer->async_wait(
            [self = shared_from_this()](const asio::error_code& error)
            {
                if (!error && self->m_heartbeatRunning.load())
                {
                    self->sendHeartbeat();
                    self->scheduleHeartbeat();
                }
            });
    }

    /**
     * @brief 发送心跳包
     */
    void sendHeartbeat()
    {
        if (m_state != ClientState::AUTHENTICATED && m_state != ClientState::IN_GAME)
        {
            return;
        }

        utils::LOG_DEBUG("♥ Sending heartbeat to server");
        std::vector<uint8_t> emptyPayload;

        asio::co_spawn(
            utils::ThreadPool::getInstance().get_executor(),
            [self = shared_from_this(), emptyPayload]() -> asio::awaitable<void>
            {
                co_await self->m_networkClient->sendPacket(static_cast<uint16_t>(RequestType::HEARTBEAT), emptyPayload);
                co_return;
            },
            asio::detached);
    }

    // ==================== 成员变量 ====================

    std::shared_ptr<NetWorkClient> m_networkClient;
    entt::poly<IServerMessageHandler> m_messageHandler;

    std::atomic<ClientState> m_state{ClientState::DISCONNECTED};

    std::string m_playerName;
    uint32_t m_clientId{0};
    uint32_t m_playerEntity{0};

    // 心跳管理
    std::atomic<bool> m_heartbeatRunning{false};
    std::shared_ptr<asio::steady_timer> m_heartbeatTimer;
    std::chrono::seconds m_heartbeatInterval{5};
};
