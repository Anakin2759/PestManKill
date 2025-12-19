/**
 * ************************************************************************
 *
 * @file GameClient.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.1
 * @brief 游戏客户端核心类（集成 UI 和 Net 模块）
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <memory>
#include <string>
#include <asio.hpp>
#include <entt/entt.hpp>
#include <spdlog/spdlog.h>

// Net 模块
#include "src/net/App/Client.h"
#include "src/net/transport/AsioUdpTransport.h"
#include "src/net/protocol/FrameCodec.h"
#include "src/net/protocol/FrameHeader.h"
// Shared 模块
#include "src/shared/messages/MessageDispatcher.h"

#include "src/shared/messages/request/ConnectedRequest.h"
#include "src/shared/messages/request/CreateRoomRequest.h"
#include "src/shared/messages/response/CreateRoomResponse.h"

// UI 模块
#include "src/ui/ui/UISystem.h"
#include "src/ui/ui/UIFactory.h"
#include "src/ui/ui/UIHelper.h"
#include "src/ui/ui/UIEvents.h"

// 客户端工具
#include <utils.h>

class GameClient
{
public:
    /**
     * @brief 构造函数
     * @param serverAddr 服务器地址
     * @param serverPort 服务器端口
     * @param playerName 玩家名称
     */
    GameClient(const std::string& serverAddr, uint16_t serverPort, const std::string& playerName)
        : m_serverAddr(serverAddr), m_serverPort(serverPort), m_playerName(playerName),
          m_ioc(std::make_unique<asio::io_context>()),
          m_transport(std::make_unique<AsioUdpTransport>(m_ioc->get_executor(), 0)) // 随机客户端端口
          ,
          m_client(std::make_unique<Client>(*m_transport, m_ioc->get_executor())),
          m_uiSystem(std::make_unique<ui::UiSystem>())
    {
        setupLogger();
        setupMessageHandlers();
        setupUIEventHandlers();
    }

    /**
     * @brief 初始化客户端
     */
    bool initialize()
    {
        try
        {
            // 启动网络接收循环
            asio::co_spawn(*m_ioc, receiveLoop(), asio::detached);

            // 启动定时更新
            startUpdateTimer();

            // 连接服务器
            return connectToServer();
        }
        catch (const std::exception& e)
        {
            m_logger->error("Client initialization failed: {}", e.what());
            return false;
        }
    }

    /**
     * @brief 更新网络 (驱动 io_context)
     */
    void updateNetwork()
    {
        if (m_ioc && !m_ioc->stopped())
        {
            m_ioc->poll();
        }
    }

    /**
     * @brief 更新 UI 系统
     */
    void updateUI(float deltaTime)
    {
        if (m_uiSystem)
        {
            m_uiSystem->update(deltaTime);
        }
    }

    /**
     * @brief 渲染 UI 系统
     */
    void renderUI(SDL_Renderer* renderer)
    {
        if (m_uiSystem)
        {
            m_uiSystem->render(renderer);
        }
    }

    /**
     * @brief 主循环
     */
    void run()
    {
        m_running = true;

        while (m_running)
        {
            // 处理网络事件
            m_ioc->poll();

            // 更新 UI
            if (m_uiSystem)
            {
                float deltaTime = calculateDeltaTime();
                m_uiSystem->update(deltaTime);
            }

            // 避免 CPU 占用过高
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    /**
     * @brief 停止客户端
     */
    void stop()
    {
        m_running = false;

        if (m_updateTimer)
        {
            m_updateTimer->cancel();
        }

        if (m_transport)
        {
            m_transport->stop();
        }

        m_ioc->stop();
        m_logger->info("Client stopped");
    }

    /**
     * @brief 发送消息到服务器
     */
    template <typename MessageType>
    void sendMessage(const MessageType& message)
    {
        auto encoded = encodeMessage(message);
        if (encoded && m_session)
        {
            m_session->send(*encoded);
            m_logger->debug("Sent message, cmd: 0x{:04X}", MessageType::CMD_ID);
        }
    }

    /**
     * @brief 获取玩家名称
     */
    const std::string& getPlayerName() const { return m_playerName; }

    /**
     * @brief 是否已连接
     */
    bool isConnected() const { return m_connected && m_session != nullptr; }

private:
    void setupLogger()
    {
        m_logger = spdlog::stdout_color_mt("GameClient");
        m_logger->set_level(spdlog::level::debug);
        m_logger->info("GameClient initialized");
    }

    void setupMessageHandlers()
    {
        // 注册连接响应处理器
        m_dispatcher.registerHandler<ConnectedRequest>(
            [this](const ConnectedRequest&) -> std::expected<std::vector<uint8_t>, MessageError>
            {
                m_logger->info("Connection acknowledged by server");
                m_connected = true;
                onConnected();
                return std::vector<uint8_t>{}; // 无需响应
            });

        // 注册创建房间响应处理器
        m_dispatcher.registerHandler<CreateRoomResponse>(
            [this](const CreateRoomResponse& resp) -> std::expected<std::vector<uint8_t>, MessageError>
            {
                if (resp.success)
                {
                    m_logger->info("Room created successfully, roomId: {}", resp.roomId);
                    onRoomCreated(resp.roomId);
                }
                else
                {
                    m_logger->warn("Room creation failed, errorCode: {}", resp.errorCode);
                    onRoomCreationFailed(resp.errorCode);
                }
                return std::vector<uint8_t>{};
            });
    }

    void setupUIEventHandlers()
    {
        auto& dispatcher = utils::Dispatcher::getInstance();

        // 订阅按钮点击事件
        dispatcher.sink<ui::events::ButtonClick>().connect<&GameClient::onUIButtonClicked>(this);
    }

    /**
     * @brief 连接到服务器
     */
    bool connectToServer()
    {
        try
        {
            auto serverEp = asio::ip::udp::endpoint(asio::ip::make_address(m_serverAddr), m_serverPort);

            // 生成客户端 Conv ID（基于时间戳）
            auto now = std::chrono::system_clock::now().time_since_epoch();
            uint32_t conv =
                static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count() & 0xFFFFFFFF);

            m_session = m_client->connect(conv, serverEp);
            m_logger->info("Connecting to server {}:{}, conv: {}", m_serverAddr, m_serverPort, conv);

            // 发送连接请求
            auto connectReq = ConnectedRequest::create(m_playerName, 1);
            sendMessage(connectReq);

            return true;
        }
        catch (const std::exception& e)
        {
            m_logger->error("Failed to connect to server: {}", e.what());
            return false;
        }
    }

    /**
     * @brief 网络接收循环
     */
    asio::awaitable<void> receiveLoop()
    {
        try
        {
            co_await m_transport->recvLoop([this](const auto& from, auto data) { m_client->input(from, data); });
        }
        catch (const std::exception& e)
        {
            m_logger->error("Receive loop error: {}", e.what());
        }
    }

    /**
     * @brief 启动定时更新
     */
    void startUpdateTimer()
    {
        m_updateTimer = std::make_unique<asio::steady_timer>(*m_ioc, std::chrono::milliseconds(10));

        std::function<void(const asio::error_code&)> update;
        update = [this, &update](const asio::error_code& ec)
        {
            if (!ec && m_running)
            {
                updateNetworkSession();

                m_updateTimer->expires_after(std::chrono::milliseconds(10));
                m_updateTimer->async_wait(update);
            }
        };

        m_updateTimer->async_wait(update);
    }

    /**
     * @brief 更新网络会话
     */
    void updateNetworkSession()
    {
        if (m_session)
        {
            auto now = std::chrono::steady_clock::now().time_since_epoch();
            auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

            m_client->update(static_cast<uint32_t>(now_ms));
        }
    }

    /**
     * @brief 计算帧时间间隔
     */
    float calculateDeltaTime()
    {
        auto now = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;
        return deltaTime;
    }

    // ========== 网络事件回调 ==========

    void onConnected()
    {
        m_logger->info("Successfully connected to server");
        // TODO: 显示主界面
    }

    void onRoomCreated(uint32_t roomId)
    {
        m_currentRoomId = roomId;
        m_logger->info("Joined room: {}", roomId);
        // TODO: 切换到房间界面
    }

    void onRoomCreationFailed(uint8_t errorCode)
    {
        m_logger->warn("Room creation failed: {}", errorCode);
        // TODO: 显示错误提示
    }

    // ========== UI 事件回调 ==========

    void onUIButtonClicked(const ui::events::ButtonClick& event)
    {
        m_logger->debug("Button clicked: {}", static_cast<uint32_t>(event.entity));
        // TODO: 处理按钮点击
    }

private:
    // 网络相关
    std::string m_serverAddr;
    uint16_t m_serverPort;
    std::string m_playerName;

    std::unique_ptr<asio::io_context> m_ioc;
    std::unique_ptr<AsioUdpTransport> m_transport;
    std::unique_ptr<Client> m_client;
    std::shared_ptr<KcpSession> m_session;

    std::unique_ptr<asio::steady_timer> m_updateTimer;
    MessageDispatcher m_dispatcher;

    bool m_connected = false;
    bool m_running = false;
    uint32_t m_currentRoomId = 0;

    // UI 相关
    std::unique_ptr<ui::UiSystem> m_uiSystem;
    std::chrono::steady_clock::time_point m_lastFrameTime = std::chrono::steady_clock::now();

    // 日志
    std::shared_ptr<spdlog::logger> m_logger;
};
