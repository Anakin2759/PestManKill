/**
 * ************************************************************************
 *
 * @file NetworkReceiveSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-01
 * @version 0.1
 * @brief 基于UDP的网络消息接收系统
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include "src/server/context/GameContext.h"
#include <asio.hpp>

class NetworkReceiveSystem
{
public:
    explicit NetworkReceiveSystem(GameContext& context) : m_context(&context) {};
    void registerEvents() {};
    void unregisterEvents() {};

private:


    GameContext* m_context;
};