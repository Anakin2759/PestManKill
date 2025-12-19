/**
 * ************************************************************************
 *
 * @file KcpSession.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.1
 * @brief KCP 会话层抽象

 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once
#include "../transport/IUdpTransport.h"
#include <ikcp.h>
#include <asio.hpp>
#include <asio/experimental/channel.hpp>
#include <expected>
#include <span>
#include <memory>
#include <vector>

class KcpSession : public std::enable_shared_from_this<KcpSession>
{
    // 定义协程通道：传递接收到的二进制包
    using Packet = std::vector<uint8_t>;
    using DataChannel = asio::experimental::channel<asio::any_io_executor, void(std::error_code, Packet)>;

public:
    /**
     * @brief 构造函数
     * @param conv 会话的 Conv ID
     * @param transport 底层 UDP 传输实现
     * @param peer 对端 UDP 地址
     * @param exec 执行器（通常是 IO 上下文的执行器）
     */
    KcpSession(uint32_t conv, IUdpTransport& transport, asio::ip::udp::endpoint peer, asio::any_io_executor exec);

    ~KcpSession();

    // 禁止拷贝
    KcpSession(const KcpSession&) = delete;
    KcpSession& operator=(const KcpSession&) = delete;

    /**
     * @brief 供 Endpoint 调用：喂入底层 UDP 数据
     */
    void input(std::span<const uint8_t> data);

    /**
     * @brief 协程接口：异步接收一个 KCP 完整包
     * @return C++23 std::expected，成功返回数据，失败返回错误码
     */
    asio::awaitable<std::expected<Packet, std::error_code>> recv();

    void send(std::span<const uint8_t> data);
    /**
     * @brief 更新 KCP 状态，需定期调用
     * @param now 当前时间戳（毫秒）
     */
    void update(uint32_t now);

    // 获取下一次更新的时间点
    uint32_t check(uint32_t now) const;

private:
    /**
     * @brief KCP 输出回调，将 KCP 数据发送到底层 UDP
     * @param buf 数据缓冲区
     * @param len 数据长度
     * @param kcp KCP 控制块指针
     * @param user 用户指针，指向当前 KcpSession 实例
     * @return int 0 表示成功，非 0 表示失败
     */
    static int kcpOutput(const char* buf, int len, ikcpcb* kcp, void* user);

private:
    ikcpcb* m_kcp{nullptr};
    IUdpTransport& m_transport;
    asio::ip::udp::endpoint m_peer;
    DataChannel m_channel;
};