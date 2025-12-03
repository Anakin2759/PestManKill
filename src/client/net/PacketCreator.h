/**
 * ************************************************************************
 *
 * @file PacketCreator.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-01
 * @version 0.1
 * @brief 数据包创建器定义
  用于创建各种类型的数据包
  提供便捷的静态方法构造数据包内容
  支持基础数据类型和自定义消息结构体
  客户端实现
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#include <nlohmann/json.hpp>
#include "src/shared/common/Common.h"

class PacketCreator
{
public:
    // 禁用构造和析构 - 纯静态工具类
    PacketCreator() = delete;
    ~PacketCreator() = delete;
    PacketCreator(const PacketCreator&) = delete;
    PacketCreator& operator=(const PacketCreator&) = delete;
    PacketCreator(PacketCreator&&) = delete;
    PacketCreator& operator=(PacketCreator&&) = delete;

    // ========== 通用数据包创建方法 ==========

    /**
     * @brief 创建包含JSON payload的数据包
     * @param type 消息类型
     * @param jsonData JSON数据
     * @return 完整的数据包(不含Header,由NetworkManager添加)
     */
    static std::vector<uint8_t> createJsonPacket(MessageType type, const nlohmann::json& jsonData)
    {
        std::string jsonStr = jsonData.dump();
        std::vector<uint8_t> payload(jsonStr.begin(), jsonStr.end());
        return payload;
    }

    /**
     * @brief 创建包含二进制数据的数据包
     * @param type 消息类型
     * @param data 二进制数据
     * @return 完整的数据包(不含Header)
     */
    static std::vector<uint8_t> createBinaryPacket(MessageType type, const std::vector<uint8_t>& data) { return data; }

    /**
     * @brief 创建空数据包(只有类型,无payload)
     * @param type 消息类型
     * @return 空数据包
     */
    static std::vector<uint8_t> createEmptyPacket(MessageType type) { return {}; }

    /**
     * @brief 创建包含POD结构体的数据包
     * @tparam T POD类型
     * @param type 消息类型
     * @param data 结构体数据
     * @return 完整的数据包(不含Header)
     */
    template <typename T>
    static std::vector<uint8_t> createStructPacket(MessageType type, const T& data)


    
    {
        static_assert(std::is_standard_layout_v<T>, "T must be a POD type");

        std::vector<uint8_t> payload(sizeof(T));
        std::memcpy(payload.data(), &data, sizeof(T));
        return payload;
    }

    // ========== 具体消息类型的快捷创建方法 ==========

    /**
     * @brief 创建心跳包
     */
    static std::vector<uint8_t> createHeartbeat() { return createEmptyPacket(MessageType::HEARTBEAT); }

    /**
     * @brief 创建登录请求包
     * @param username 用户名
     * @param password 密码
     */
    static std::vector<uint8_t> createLoginRequest(const std::string& username, const std::string& password)
    {
        nlohmann::json loginData = {{"username", username}, {"password", password}};
        return createJsonPacket(MessageType::LOGIN, loginData);
    }

    /**
     * @brief 创建登出请求包
     */
    static std::vector<uint8_t> createLogoutRequest() { return createEmptyPacket(MessageType::LOGOUT); }

    /**
     * @brief 创建使用卡牌消息包
     * @param playerId 玩家ID
     * @param cardId 卡牌ID
     * @param targets 目标列表
     */
    static std::vector<uint8_t>
        createUseCardMessage(uint32_t playerId, uint32_t cardId, const std::vector<uint32_t>& targets)
    {
        nlohmann::json cardData = {{"player", playerId}, {"card", cardId}, {"targets", targets}};
        return createJsonPacket(MessageType::USE_CARD, cardData);
    }

    /**
     * @brief 创建抽卡消息包
     * @param playerId 玩家ID
     * @param count 抽卡数量
     */
    static std::vector<uint8_t> createDrawCardMessage(uint32_t playerId, uint32_t count = 1)
    {
        nlohmann::json drawData = {{"player", playerId}, {"count", count}};
        return createJsonPacket(MessageType::DRAW_CARD, drawData);
    }

    /**
     * @brief 创建结束回合消息包
     * @param playerId 玩家ID
     */
    static std::vector<uint8_t> createEndTurnMessage(uint32_t playerId)
    {
        nlohmann::json endTurnData = {{"player", playerId}};
        return createJsonPacket(MessageType::END_TURN, endTurnData);
    }

    /**
     * @brief 创建聊天消息包
     * @param senderId 发送者ID
     * @param message 消息内容
     * @param channelId 频道ID (可选)
     */
    static std::vector<uint8_t> createChatMessage(uint32_t senderId, const std::string& message, uint32_t channelId = 0)
    {
        nlohmann::json chatData = {{"sender", senderId}, {"message", message}, {"channel", channelId}};
        return createJsonPacket(MessageType::CHAT_MESSAGE, chatData);
    }

    /**
     * @brief 创建错误消息包
     * @param errorCode 错误码
     * @param errorMessage 错误描述
     */
    static std::vector<uint8_t> createErrorMessage(uint32_t errorCode, const std::string& errorMessage)
    {
        nlohmann::json errorData = {{"code", errorCode}, {"message", errorMessage}};
        return createJsonPacket(MessageType::ERROR_MESSAGE, errorData);
    }

    // ========== 数据包解析方法 ==========

    /**
     * @brief 从数据包中解析JSON数据
     * @param payload 数据包payload部分
     * @return JSON对象
     */
    static nlohmann::json parseJsonPacket(const std::vector<uint8_t>& payload)
    {
        if (payload.empty())
        {
            return nlohmann::json::object();
        }
        std::string jsonStr(payload.begin(), payload.end());
        return nlohmann::json::parse(jsonStr);
    }

    /**
     * @brief 从数据包中解析JSON数据(原始指针版本)
     * @param data 数据指针
     * @param size 数据大小
     * @return JSON对象
     */
    static nlohmann::json parseJsonPacket(const uint8_t* data, std::size_t size)
    {
        if (data == nullptr || size == 0)
        {
            return nlohmann::json::object();
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        std::string jsonStr(reinterpret_cast<const char*>(data), size);
        return nlohmann::json::parse(jsonStr);
    }

    /**
     * @brief 从数据包中解析POD结构体
     * @tparam T POD类型
     * @param payload 数据包payload部分
     * @return 结构体数据
     */
    template <typename T>
    static T parseStructPacket(const std::vector<uint8_t>& payload)
    {
        static_assert(std::is_standard_layout_v<T>, "T must be a POD type");

        if (payload.size() < sizeof(T))
        {
            throw std::runtime_error("Payload size is smaller than struct size");
        }

        T data;
        std::memcpy(&data, payload.data(), sizeof(T));
        return data;
    }

    /**
     * @brief 从数据包中解析POD结构体(原始指针版本)
     * @tparam T POD类型
     * @param data 数据指针
     * @param size 数据大小
     * @return 结构体数据
     */
    template <typename T>
    static T parseStructPacket(const uint8_t* data, std::size_t size)
    {
        static_assert(std::is_standard_layout_v<T>, "T must be a POD type");

        if (size < sizeof(T))
        {
            throw std::runtime_error("Payload size is smaller than struct size");
        }

        T structData;
        std::memcpy(&structData, data, sizeof(T));
        return structData;
    }

    // ========== 辅助方法 ==========

    /**
     * @brief 获取消息类型的字符串表示
     * @param type 消息类型
     * @return 类型名称
     */
    static const char* getMessageTypeName(MessageType type)
    {
        switch (type)
        {
            case MessageType::HEARTBEAT:
                return "HEARTBEAT";
            case MessageType::LOGIN:
                return "LOGIN";
            case MessageType::LOGOUT:
                return "LOGOUT";
            case MessageType::USE_CARD:
                return "USE_CARD";
            case MessageType::DRAW_CARD:
                return "DRAW_CARD";
            case MessageType::END_TURN:
                return "END_TURN";
            case MessageType::CHAT_MESSAGE:
                return "CHAT_MESSAGE";
            case MessageType::GAME_STATE:
                return "GAME_STATE";
            case MessageType::ERROR_MESSAGE:
                return "ERROR_MESSAGE";
            case MessageType::ACK:
                return "ACK";
            default:
                return "UNKNOWN";
        }
    }

    /**
     * @brief 将MessageType转换为uint16_t
     */
    static uint16_t toUint16(MessageType type) { return static_cast<uint16_t>(type); }

    /**
     * @brief 将uint16_t转换为MessageType
     */
    static MessageType fromUint16(uint16_t value) { return static_cast<MessageType>(value); }
};