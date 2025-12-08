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
#include "net/IServerResponseHandler.h"
#include "src/client/utils/utils.h"
#include "src/client/net/ServerResponseHandler.h"
#define STB_IMAGE_IMPLEMENTATION
int main()
{
#if defined(_MSC_VER)
    // 设置控制台为 UTF-8 编码（Windows 特有）
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    LOG_INFO("========================================");
    LOG_INFO("Client Startup - Heartbeat Communication Test");
    LOG_INFO("========================================");

    // 创建 GameClient
    auto gameClient = std::make_shared<GameClient>();

    // 设置消息处理器
    ServerResponseHandler messageHandler;
    gameClient->setMessageHandler(entt::poly<IServerResponseHandler>{std::move(messageHandler)});

    // 连接到服务器（异步执行）
    std::string serverHost = "127.0.0.1";
    constexpr uint16_t SERVER_PORT = 8888;

    LOG_INFO("Connecting to server: {}:{}", serverHost, SERVER_PORT);

    // 使用 co_spawn 在 ThreadPool 上执行异步连接
    asio::co_spawn(
        utils::ThreadPool::getInstance().get_executor(),
        [gameClient, serverHost, playerName = std::string("TestPlayer")]() -> asio::awaitable<void>
        {
            // 连接到服务器
            co_await gameClient->connect(serverHost, SERVER_PORT);

            // 登录
            LOG_INFO("Sending login request: {}", playerName);
            co_await gameClient->login(playerName);
        },
        asio::detached);

    // Main loop - keep running and receive heartbeat responses
    LOG_INFO("Client is running...");
    LOG_INFO("Heartbeat packets will be sent automatically (every 5 seconds after login)");
    LOG_INFO("Press Ctrl+C to stop the client");

    // 保持程序运行
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
