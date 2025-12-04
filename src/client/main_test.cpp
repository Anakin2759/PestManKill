/**
 * ************************************************************************
 *
 * @file main_test.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-04
 * @version 0.1
 * @brief 客户端主程序入口 - 心跳包通信测试
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <nlohmann/json.hpp>
#include "src/client/net/GameClient.h"
#include "src/client/utils/Logger.h"
#include "net/IServerMessageHandler.h"

std::atomic<bool> g_running{true};

void signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        std::cout << "\nReceived stop signal, shutting down client..." << std::endl;
        g_running.store(false);
    }
}

/**
 * @brief 简单的消息处理器实现
 */
class TestMessageHandler
{
public:
    void onConnected() { utils::LOG_INFO("✅ Connected to server"); }

    void onDisconnected() { utils::LOG_INFO("❌ Disconnected from server"); }

    void onLoginSuccess(uint32_t clientId, uint32_t playerEntity)
    {
        utils::LOG_INFO("✅ Login successful! clientId={}, playerEntity={}", clientId, playerEntity);
    }

    void onLoginFailed(const std::string& reason) { utils::LOG_ERROR("❌ Login failed: {}", reason); }

    void onGameStart() { utils::LOG_INFO("🎮 Game started"); }

    void onGameEnd(const std::string& reason) { utils::LOG_INFO("🏁 Game ended: {}", reason); }
    void onPlayerJoined(const std::string& playerName) { utils::LOG_INFO("👤 Player joined: {}", playerName); }

    void onPlayerLeft(const std::string& playerName) { utils::LOG_INFO("👋 Player left: {}", playerName); }

    void onPlayerDisconnected(const std::string& playerName)
    {
        utils::LOG_WARN("⚠️  Player disconnected: {}", playerName);
    }

    void onPlayerReconnected(const std::string& playerName)
    {
        utils::LOG_INFO("🔄 Player reconnected: {}", playerName);
    }

    void onGameStateUpdate(const nlohmann::json& gameState)
    {
        utils::LOG_INFO("📦 Game state updated: {}", gameState.dump());
    }

    void onUseCardResponse(bool success, const std::string& message)
    {
        if (success)
        {
            utils::LOG_INFO("✅ Card used successfully: {}", message);
        }
        else
        {
            utils::LOG_WARN("❌ Failed to use card: {}", message);
        }
    }

    void onBroadcastEvent(const std::string& eventMessage) { utils::LOG_INFO("📢 Broadcast event: {}", eventMessage); }

    void onError(const std::string& errorMessage) { utils::LOG_ERROR("❌ Server error: {}", errorMessage); }
};

int main()
{
    // 注册信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
#if defined(_MSC_VER)
    // 设置控制台为 UTF-8 编码（Windows 特有）
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    try
    {
        utils::LOG_INFO("========================================");
        utils::LOG_INFO("Client Startup - Heartbeat Communication Test");
        utils::LOG_INFO("========================================");

        // 创建 GameClient
        auto gameClient = std::make_shared<GameClient>();

        // 设置消息处理器
        TestMessageHandler messageHandler;
        gameClient->setMessageHandler(entt::poly<IServerMessageHandler>{std::move(messageHandler)});

        // 连接到服务器
        std::string serverHost = "127.0.0.1";
        constexpr uint16_t SERVER_PORT = 8888;

        utils::LOG_INFO("Connecting to server: {}:{}", serverHost, SERVER_PORT);
        gameClient->connect(serverHost, SERVER_PORT);

        // Wait for connection establishment
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Login
        std::string playerName = "TestPlayer";
        utils::LOG_INFO("Sending login request: {}", playerName);

        asio::co_spawn(
            utils::ThreadPool::getInstance().get_executor(),
            [gameClient, playerName]() -> asio::awaitable<void>
            {
                co_await gameClient->login(playerName);
                gameClient->startHeartbeat(std::chrono::seconds(5));
                co_return;
            },
            asio::detached);

        // Main loop - keep running and receive heartbeat responses
        utils::LOG_INFO("Client is running...");
        utils::LOG_INFO("Heartbeat packets will be sent automatically (every 5 seconds after login)");
        utils::LOG_INFO("Press Ctrl+C to stop the client");

        int counter = 0;
        while (g_running.load())
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            counter++;
            if (counter % 10 == 0)
            {
                auto state = gameClient->getState();
                const char* stateStr = "UNKNOWN";
                switch (state)
                {
                    case ClientState::DISCONNECTED:
                        stateStr = "DISCONNECTED";
                        break;
                    case ClientState::CONNECTING:
                        stateStr = "CONNECTING";
                        break;
                    case ClientState::CONNECTED:
                        stateStr = "CONNECTED";
                        break;
                    case ClientState::AUTHENTICATED:
                        stateStr = "AUTHENTICATED";
                        break;
                    case ClientState::IN_GAME:
                        stateStr = "IN_GAME";
                        break;
                }
                // utils::LOG_INFO("当前状态: {} | ClientID: {} | EntityID: {}",
                //                 stateStr,
                //                 gameClient->getClientId(),
                //                 gameClient->getPlayerEntity());
            }
        }

        // Disconnect
        utils::LOG_INFO("Disconnecting...");
        gameClient->disconnect();

        // Wait for resource cleanup
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        utils::LOG_INFO("Client stopped");
    }
    catch (const std::exception& e)
    {
        std::cerr << "客户端异常: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
