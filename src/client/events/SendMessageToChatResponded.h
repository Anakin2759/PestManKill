/**
 * ************************************************************************
 *
 * @file SendMessageToChatResponded.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-04
 * @version 0.1
 * @brief 聊天消息响应事件
 *        当收到服务器发送的聊天消息时触发
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <string>
#include <cstdint>
#include <nlohmann/json.hpp>

/**
 * @brief 聊天消息响应事件
 * 从服务器接收到其他玩家的聊天消息时触发
 */
struct SendMessageToChatResponded
{
    entt::entity sender;    // 发送者的玩家实体 ID
    std::string senderName; // 发送者的玩家名称（如果可用）
    std::string message;    // 聊天消息内容
};