/**
 * ************************************************************************
 *
 * @file NetWorkManager.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-02
 * @version 0.1
 * @brief 网络管理器定义
 * 基于 ASIO + C++23 协程的 UDP 网络管理器
 * - 完全由 ThreadPool 驱动
 * - 支持异步发送/接收
 * - 支持可靠传输（ACK机制，事件驱动）
 * - 服务器模式
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <asio.hpp>
#include <array>

class NetWorkManager : public std::enable_shared_from_this<NetWorkManager>
{
};