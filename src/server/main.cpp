/**
 * ************************************************************************
 *
 * @file main.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-01
 * @version 0.1
 * @brief 服务器主程序入口 - 心跳包通信测试
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
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include "src/server/net/NetWorkManager.h"
#include "src/server/net/ClientSessionManager.h"
#include "src/shared/common/Common.h"
#include "src/shared/messages/request/UseCardRequest.h"
#include "src/shared/messages/request/SettlementRequest.h"
#include "src/shared/messages/response/UseCardResponse.h"
#include "src/shared/messages/response/SettlementResponse.h"
#include <nlohmann/json.hpp>
#include <entt/entt.hpp>

std::atomic<bool> g_running{true};

void signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        std::cout << "\nReceived stop signal, shutting down server..." << std::endl;
        g_running.store(false);
    }
}

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
        GameContext context;
        auto& logger = context.logger;

        logger->info("========================================");
        logger->info("Server Startup - Heartbeat Communication Test");
        logger->info("========================================");

        // 创建线程池
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
        auto& threadPool = context.threadPool;

        // 创建网络管理器
        auto networkManager = std::make_shared<NetWorkManager>(context);

        // 创建客户端会话管理器
        auto sessionManager = std::make_shared<ClientSessionManager>(logger);

        // 设置消息处理器
        networkManager->setPacketHandler(
            [&logger, &sessionManager, &networkManager, &threadPool](
                uint16_t type, const uint8_t* payload, size_t size, const asio::ip::udp::endpoint& sender)
            {
                auto requestType = static_cast<RequestType>(type);

                // 更新心跳
                sessionManager->updateHeartbeat(sender);

                logger->debug("Received message: type=0x{:04X}, size={}, from={}:{}",
                              type,
                              size,
                              sender.address().to_string(),
                              sender.port());

                switch (requestType)
                {
                    case RequestType::LOGIN:
                    {
                        // 解析玩家名称
                        if (size == 0U)
                        {
                            logger->warn("Login request missing payload");
                            break;
                        }

                        std::string playerName(reinterpret_cast<const char*>(payload), size);

                        logger->info("Player login: {}", playerName);

                        // 生成客户端 ID 和玩家实体 ID（简化版）
                        static uint32_t nextClientId = 1;
                        static uint32_t nextEntityId = 100;

                        uint32_t clientId = nextClientId++;
                        uint32_t entityId = nextEntityId++;

                        // 注册会话
                        sessionManager->registerClient(sender, entt::entity(entityId), playerName);

                        // 发送登录响应（使用 ResponseType::OK）
                        nlohmann::json response;
                        response["clientId"] = clientId;
                        response["entityId"] = entityId;
                        response["message"] = "Login successful";

                        std::string jsonStr = response.dump();
                        std::vector<uint8_t> payload_data(jsonStr.begin(), jsonStr.end());

                        asio::co_spawn(
                            threadPool.get_executor(),
                            [networkManager, sender, payload_data]() -> asio::awaitable<void>
                            {
                                co_await networkManager->sendReliablePacket(
                                    sender, static_cast<uint16_t>(ResponseType::OK), payload_data);
                            },
                            asio::detached);

                        break;
                    }

                    case RequestType::HEARTBEAT:
                    {
                        logger->debug("Received heartbeat from {}:{}", sender.address().to_string(), sender.port());

                        // 回复心跳（使用 ResponseType::OK，空载荷）
                        std::vector<uint8_t> emptyPayload;
                        asio::co_spawn(
                            threadPool.get_executor(),
                            [networkManager, sender, emptyPayload]() -> asio::awaitable<void>
                            {
                                co_await networkManager->sendPacket(
                                    sender, static_cast<uint16_t>(ResponseType::OK), emptyPayload);
                            },
                            asio::detached);
                        break;
                    }

                    case RequestType::LOGOUT:
                    {
                        auto entity = sessionManager->getPlayerEntity(sender);
                        if (entity.has_value())
                        {
                            sessionManager->removeClient(sender);
                        }
                        break;
                    }

                    case RequestType::USE_CARD:
                    {
                        // 解析使用卡牌请求
                        if (size == 0U)
                        {
                            logger->warn("UseCard request missing payload");
                            break;
                        }

                        std::string jsonStr(reinterpret_cast<const char*>(payload), size);
                        try
                        {
                            auto json = nlohmann::json::parse(jsonStr);
                            auto request = UseCardRequest::fromJson(json);

                            logger->info("UseCard request: player={}, card={}, targets={}",
                                         request.player,
                                         request.card,
                                         request.targets.size());

                            // TODO: 验证请求有效性（玩家是否拥有该卡牌、目标是否有效等）

                            // 创建响应消息
                            UseCardResponse response{.player = request.player,
                                                     .card = request.card,
                                                     .targets = request.targets,
                                                     .success = true,
                                                     .message = "Card used successfully"};

                            std::string responseJson = response.toJson().dump();
                            std::vector<uint8_t> responsePayload(responseJson.begin(), responseJson.end());

                            // 广播给所有客户端
                            auto endpoints = sessionManager->getAllEndpoints(true);
                            for (const auto& endpoint : endpoints)
                            {
                                asio::co_spawn(
                                    threadPool.get_executor(),
                                    [networkManager, endpoint, responsePayload]() -> asio::awaitable<void>
                                    {
                                        co_await networkManager->sendReliablePacket(
                                            endpoint,
                                            static_cast<uint16_t>(ResponseType::USE_CARD_RESULT),
                                            responsePayload);
                                    },
                                    asio::detached);
                            }

                            logger->info("UseCard response broadcasted to {} clients", endpoints.size());
                        }
                        catch (const std::exception& e)
                        {
                            logger->error("Failed to parse UseCard request: {}", e.what());
                        }
                        break;
                    }

                    case RequestType::SETTLEMENT:
                    {
                        // 解析结算请求
                        if (size == 0U)
                        {
                            logger->warn("Settlement request missing payload");
                            break;
                        }

                        std::string jsonStr(reinterpret_cast<const char*>(payload), size);
                        try
                        {
                            auto json = nlohmann::json::parse(jsonStr);
                            auto request = SettlementRequest::fromJson(json);

                            logger->info("Settlement request: player={}, card={}, target={}",
                                         request.player,
                                         request.card,
                                         request.target);

                            // TODO: 执行实际的结算逻辑

                            // 创建结算响应
                            SettlementResponse response{.player = request.player,
                                                        .card = request.card,
                                                        .target = request.target,
                                                        .success = true,
                                                        .message = "Settlement completed"};

                            std::string responseJson = response.toJson().dump();
                            std::vector<uint8_t> responsePayload(responseJson.begin(), responseJson.end());

                            // 广播给所有客户端
                            auto endpoints = sessionManager->getAllEndpoints(true);
                            for (const auto& endpoint : endpoints)
                            {
                                asio::co_spawn(
                                    threadPool.get_executor(),
                                    [networkManager, endpoint, responsePayload]() -> asio::awaitable<void>
                                    {
                                        co_await networkManager->sendReliablePacket(
                                            endpoint,
                                            static_cast<uint16_t>(ResponseType::SETTLEMENT_RESULT),
                                            responsePayload);
                                    },
                                    asio::detached);
                            }

                            logger->info("Settlement response broadcasted to {} clients", endpoints.size());
                        }
                        catch (const std::exception& e)
                        {
                            logger->error("Failed to parse Settlement request: {}", e.what());
                        }
                        break;
                    }

                    default:
                        logger->warn("Unhandled message type: {}", type);
                        break;
                }
            });

        // 启动服务器
        constexpr uint16_t SERVER_PORT = 8888;
        networkManager->start(SERVER_PORT);

        logger->info("Server is running, listening on port: {}", SERVER_PORT);
        logger->info("Press Ctrl+C to stop the server");

        // 心跳超时检查循环
        while (g_running.load())
        {
            std::this_thread::sleep_for(std::chrono::seconds(5));

            // 检查超时的客户端
            auto timedOutClients = sessionManager->checkTimeouts();
            for (auto entity : timedOutClients)
            {
                logger->warn("Client timeout: entity={}", static_cast<uint32_t>(entity));
            }

            // Print online client count
            auto endpoints = sessionManager->getAllEndpoints();
            logger->info("Current online clients: {}", endpoints.size());
        }

        // Stop server
        logger->info("Stopping server...");
        networkManager->stop();
        threadPool.join();

        logger->info("Server stopped");
    }
    catch (const std::exception& e)
    {
        std::cerr << "Server exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
