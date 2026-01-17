/**
 * ************************************************************************
 *
 * @file main.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-01
 * @version 0.1
 * @brief 服务器主程序入口 - 心跳包通信测试
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <asio.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <nlohmann/json.hpp>
#include <entt/entt.hpp>

std::atomic<bool> g_running{true};

void signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        std::cout << "\nReceived stop signal, shutting down server..." << std::endl;
        g_running.store(false);
    }
}

int main()
{
    // 注册信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
#if defined(_MSC_VER)
    // 设置控制台为 UTF-8 编码（Windows 特有）
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::setlocale(LC_ALL, "C");
    // 强制 C++ I/O 流语言环境为 "C" (英文)
    std::locale::global(std::locale("C"));
#endif
}
