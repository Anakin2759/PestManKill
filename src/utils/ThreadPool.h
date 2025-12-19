#pragma once

#include <asio/thread_pool.hpp>
#include <thread>

namespace utils
{

class ThreadPool
{
public:
    // 获取全局线程池实例(单例模式)
    static asio::thread_pool& getInstance(size_t threadCount = std::thread::hardware_concurrency())
    {
        static asio::thread_pool instance(threadCount);
        return instance;
    }

    // 禁止拷贝和移动
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

private:
    ThreadPool() = default;
    ~ThreadPool() = default;
};
} // namespace utils