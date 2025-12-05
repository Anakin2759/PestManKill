/**
 * ************************************************************************
 *
 * @file IServerMessageHandler.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief 服务器消息处理接口定义
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <entt/entt.hpp>
/**
 * @brief 服务器消息回调接口
 */
struct IServerMessageHandler : entt::type_list<>
{
    template <typename Base>
    struct type : Base
    {
        void onConnected() { entt::poly_call<0>(*this); }
        void onDisconnected() { entt::poly_call<1>(*this); }
        void onLoginSuccess(uint32_t clientId, uint32_t playerEntity)
        {
            entt::poly_call<2>(*this, clientId, playerEntity);
        }
        void onLoginFailed(const std::string& reason) { entt::poly_call<3>(*this, reason); }
        void onGameStart() { entt::poly_call<4>(*this); }
        void onGameEnd(const std::string& reason) { entt::poly_call<5>(*this, reason); }
        void onPlayerJoined(const std::string& playerName) { entt::poly_call<6>(*this, playerName); }
        void onPlayerLeft(const std::string& playerName) { entt::poly_call<7>(*this, playerName); }
        void onPlayerDisconnected(const std::string& playerName) { entt::poly_call<8>(*this, playerName); }
        void onPlayerReconnected(const std::string& playerName) { entt::poly_call<9>(*this, playerName); }
        void onGameStateUpdate(const nlohmann::json& gameState) { entt::poly_call<10>(*this, gameState); }
        void onUseCardResponse(bool success, const std::string& message)
        {
            entt::poly_call<11>(*this, success, message);
        }
        void onBroadcastEvent(const std::string& eventMessage) { entt::poly_call<12>(*this, eventMessage); }
        void onError(const std::string& errorMessage) { entt::poly_call<13>(*this, errorMessage); }
    };

    template <typename T>
    using impl = entt::value_list<&T::onConnected,
                                  &T::onDisconnected,
                                  &T::onLoginSuccess,
                                  &T::onLoginFailed,
                                  &T::onGameStart,
                                  &T::onGameEnd,
                                  &T::onPlayerJoined,
                                  &T::onPlayerLeft,
                                  &T::onPlayerDisconnected,
                                  &T::onPlayerReconnected,
                                  &T::onGameStateUpdate,
                                  &T::onUseCardResponse,
                                  &T::onBroadcastEvent,
                                  &T::onError>;
};