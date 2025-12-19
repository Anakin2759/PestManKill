/**
 * ************************************************************************
 *
 * @file Messages.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.1
 * @brief 所有消息类型的统一包含头文件
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

// 基础定义
#include "MessageBase.h"
#include "MessageDispatcher.h"
#include "src/shared/common/CommandID.h"

// 请求消息
#include "request/ConnectedRequest.h"
#include "request/CreateRoomRequest.h"
#include "request/DiscardCardRequest.h"
#include "request/SendMessageToChatRequest.h"
#include "request/SettlementRequest.h"
#include "request/UseCardRequest.h"

// 响应消息
#include "response/CreateRoomResponse.h"
#include "response/DiscardCardResponse.h"
#include "response/SendMessageToChatResponse.h"
#include "response/SettlementResponse.h"
#include "response/UseCardResponse.h"
