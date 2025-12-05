/**
 * ************************************************************************
 *
 * @file IServerMessageHandler.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief 服务器消息处理接口定义
    定义了客户端处理服务器消息的回调接口。
    包括连接状态、登录结果、游戏状态更新等多种消息类型的处理方法。
    - 提供连接方法
    - 提供登录结果处理方法
    - 提供游戏状态更新处理方法
    - 提供错误处理方法
    - 支持多种消息类型的回调处理
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
 * @brief 服务器消息响应回调接口
 */
struct IServerResponseHandler : entt::type_list<>
{
    template <typename Base>
    struct type : Base
    {
        /**
         * @brief 处理连接成功事件
         */
        void onConnected() { entt::poly_call<0>(*this); }
        /**
         * @brief 处理断开连接事件
         */
        void onDisconnected() { entt::poly_call<1>(*this); }
        /**
         * @brief 处理登录成功事件
         * @param clientId 客户端唯一标识
         * @param playerEntity 玩家实体ID
         */
        void onLoginSuccess(uint32_t clientId, uint32_t playerEntity)
        {
            entt::poly_call<2>(*this, clientId, playerEntity);
        }
        /**
         * @brief 处理登录失败事件
         * @param reason 失败原因描述
         */
        void onLoginFailed(const std::string& reason) { entt::poly_call<3>(*this, reason); }
        /**
         * @brief 处理游戏开始事件
         */
        void onGameStart() { entt::poly_call<4>(*this); }
        /**
         * @brief 处理游戏结束事件
         * @param reason 游戏结束的原因描述
         */
        void onGameEnd(const std::string& reason) { entt::poly_call<5>(*this, reason); }
        /**
         * @brief 处理玩家加入事件
         * @param playerName 加入的玩家名称
         */
        void onPlayerJoined(const std::string& playerName) { entt::poly_call<6>(*this, playerName); }
        /**
         * @brief 处理玩家离开事件
         * @param playerName 离开的玩家名称
         */
        void onPlayerLeft(const std::string& playerName) { entt::poly_call<7>(*this, playerName); }
        /**
         * @brief 处理玩家断开连接事件
         * @param playerName 断开连接的玩家名称
         */
        void onPlayerDisconnected(const std::string& playerName) { entt::poly_call<8>(*this, playerName); }
        /**
         * @brief 处理玩家重新连接事件
         * @param playerName 重新连接的玩家名称
         */
        void onPlayerReconnected(const std::string& playerName) { entt::poly_call<9>(*this, playerName); }
        /**
         * @brief 处理游戏状态更新
         * @param gameState 包含游戏状态的 JSON 对象
         */
        void onGameStateUpdate(const nlohmann::json& gameState) { entt::poly_call<10>(*this, gameState); }
        /**
         * @brief 处理使用卡牌的响应
         * @param success 是否成功
         * @param message 相关消息
         */
        void onUseCardResponse(bool success, const std::string& message)
        {
            entt::poly_call<11>(*this, success, message);
        }
        /**
         * @brief 处理广播事件
         * @param eventMessage 广播的事件消息
         */
        void onBroadcastEvent(const std::string& eventMessage) { entt::poly_call<12>(*this, eventMessage); }
        /**
         * @brief 处理错误事件
         * @param errorMessage 错误消息
         */
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