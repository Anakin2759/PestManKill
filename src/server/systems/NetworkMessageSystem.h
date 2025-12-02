/**
 * ************************************************************************
 *
 * @file NetworkMessageSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-01
 * @version 0.1
 * @brief 基于UDP的网络消息发送/接收系统
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include <asio.hpp>
#include <memory>
#include "src/server/context/GameContext.h"
#include "src/server/net/NetWorkManager.h"

class NetworkMessageSystem
{
public:
    explicit NetworkMessageSystem(GameContext& context)
        : m_context(&context), m_networkManager(std::make_shared<NetWorkManager>()) {};
    void registerEvents() {
        // 注册网络消息处理事件
    };
    void unregisterEvents() {
        // 注销网络消息处理事件
    };

private:
    GameContext* m_context;
    std::shared_ptr<NetWorkManager> m_networkManager;
};