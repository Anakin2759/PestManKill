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
#include <asio.hpp>
#include <nlohmann/json.hpp>
#include "src/client/net/GameClient.h"
#include "src/client/utils/Logger.h"
#include "net/IServerMessageHandler.h"

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

    void onLoginFailed(const std::string_view reason) { utils::LOG_ERROR("❌ Login failed: {}", reason); }

    void onGameStart() { utils::LOG_INFO("🎮 Game started"); }

    void onGameEnd(std::string_view reason) { utils::LOG_INFO("🏁 Game ended: {}", reason); }
    void onPlayerJoined(std::string_view playerName) { utils::LOG_INFO("👤 Player joined: {}", playerName); }

    void onPlayerLeft(std::string_view playerName) { utils::LOG_INFO("👋 Player left: {}", playerName); }

    void onPlayerDisconnected(std::string_view playerName)
    {
        utils::LOG_WARN("⚠️  Player disconnected: {}", playerName);
    }

    void onPlayerReconnected(std::string_view playerName) { utils::LOG_INFO("🔄 Player reconnected: {}", playerName); }

    void onGameStateUpdate(const nlohmann::json& gameState)
    {
        utils::LOG_INFO("📦 Game state updated: {}", gameState.dump());
    }

    void onUseCardResponse(bool success, std::string_view message)
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
#if defined(_MSC_VER)
    // 设置控制台为 UTF-8 编码（Windows 特有）
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    utils::LOG_INFO("========================================");
    utils::LOG_INFO("Client Startup - Heartbeat Communication Test");
    utils::LOG_INFO("========================================");

    // 创建 GameClient
    auto gameClient = std::make_shared<GameClient>();

    // 设置消息处理器
    TestMessageHandler messageHandler;
    gameClient->setMessageHandler(entt::poly<IServerMessageHandler>{std::move(messageHandler)});

    // 连接到服务器（异步执行）
    std::string serverHost = "127.0.0.1";
    constexpr uint16_t SERVER_PORT = 8888;

    utils::LOG_INFO("Connecting to server: {}:{}", serverHost, SERVER_PORT);

    // 使用 co_spawn 在 ThreadPool 上执行异步连接
    asio::co_spawn(
        utils::ThreadPool::getInstance().get_executor(),
        [gameClient, serverHost, playerName = std::string("TestPlayer")]() -> asio::awaitable<void>
        {
            // 连接到服务器
            co_await gameClient->connect(serverHost, SERVER_PORT);

            // 登录
            utils::LOG_INFO("Sending login request: {}", playerName);
            co_await gameClient->login(playerName);
        },
        asio::detached);

    // Main loop - keep running and receive heartbeat responses
    utils::LOG_INFO("Client is running...");
    utils::LOG_INFO("Heartbeat packets will be sent automatically (every 5 seconds after login)");
    utils::LOG_INFO("Press Ctrl+C to stop the client");

    // 保持程序运行
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
