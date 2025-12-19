/**
 * ************************************************************************
 *
 * @file Dispatcher.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief 全局单例事件分发器
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>

namespace utils
{
class Dispatcher
{
public:
    static entt::dispatcher& getInstance()
    {
        static entt::dispatcher instance;
        return instance;
    }

private:
    Dispatcher() = default;
    ~Dispatcher() = default;
    Dispatcher(const Dispatcher&) = delete;
    Dispatcher& operator=(const Dispatcher&) = delete;
    Dispatcher(Dispatcher&&) = delete;
    Dispatcher& operator=(Dispatcher&&) = delete;
};
} // namespace utils
