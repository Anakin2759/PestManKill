/**
 * ************************************************************************
 *
 * @file ServerResponseHandler.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-08
 * @version 0.1
 * @brief 服务器响应处理接口定义
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "IServerResponseHandler.h"
class ServerResponseHandler
{
public:
    void onConnected() { LOG_INFO("✅ Connected to server"); }

    void onDisconnected() { LOG_INFO("❌ Disconnected from server"); }

    void onLoginSuccess(uint32_t clientId, uint32_t playerEntity)
    {
        LOG_INFO("✅ Login successful! clientId={}, playerEntity={}", clientId, playerEntity);
    }

    void onLoginFailed(const std::string_view reason) { LOG_ERROR("❌ Login failed: {}", reason); }

    void onGameStart() { LOG_INFO("🎮 Game started"); }

    void onGameEnd(std::string_view reason) { LOG_INFO("🏁 Game ended: {}", reason); }
    void onPlayerJoined(std::string_view playerName) { LOG_INFO("👤 Player joined: {}", playerName); }

    void onPlayerLeft(std::string_view playerName) { LOG_INFO("👋 Player left: {}", playerName); }

    void onPlayerDisconnected(std::string_view playerName) { LOG_WARN("⚠️  Player disconnected: {}", playerName); }

    void onPlayerReconnected(std::string_view playerName) { LOG_INFO("🔄 Player reconnected: {}", playerName); }

    void onGameStateUpdate(const nlohmann::json& gameState) { LOG_INFO("📦 Game state updated: {}", gameState.dump()); }

    void onUseCardResponse(bool success, std::string_view message)
    {
        if (success)
        {
            LOG_INFO("✅ Card used successfully: {}", message);
        }
        else
        {
            LOG_WARN("❌ Failed to use card: {}", message);
        }
    }

    void onBroadcastEvent(const std::string& eventMessage) { LOG_INFO("📢 Broadcast event: {}", eventMessage); }

    void onError(const std::string& errorMessage) { LOG_ERROR("❌ Server error: {}", errorMessage); }
};