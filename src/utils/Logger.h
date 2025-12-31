
#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <source_location>
#include <mutex> // 引入 <mutex>

namespace utils
{
class Logger
{
public:
    static std::shared_ptr<spdlog::logger>& getLogger()
    {
        static std::shared_ptr<spdlog::logger> logger;
        static std::once_flag flag; // 用于一次性初始化

        std::call_once(
            flag,
            [&]()
            {
                // 1. 创建控制台 sink (stdout_color_mt)
                auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
                console_sink->set_pattern("%^[%T] [%l] %n: %v%$"); // 可选：设置控制台输出格式

                // 2. 创建文件 sink (rotating_file_sink_mt)
                // 文件路径：logs/pestmankill.log
                // 最大文件大小：5MB (1024 * 1024 * 5)
                // 最大文件数：1 (如果达到上限，旧文件会被覆盖)
                auto file_sink =
                    std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/pestmankill.log", 1024 * 1024 * 5, 1);
                file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%# %!] %v"); // 可选：设置文件输出格式

                // 3. 创建 logger
                std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
                logger = std::make_shared<spdlog::logger>("PestManKill", begin(sinks), end(sinks));

                // 可选：设置日志级别
                logger->set_level(spdlog::level::debug);
                // 可选：当日志器满了时异步刷新
                // spdlog::flush_every(std::chrono::seconds(3));
            });

        return logger;
    }

    // ... (其他删除的构造函数/赋值运算符保持不变)
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

private:
    Logger() = default;
    ~Logger() = default;
};

} // namespace utils

// ----------------- 宏包裹日志 -----------------
#ifdef LOG_INFO
#undef LOG_INFO
#endif
#define LOG_INFO(fmt, ...)                                                                                             \
    utils::Logger::getLogger()->info("[{}:{} {}] " fmt,                                                                \
                                     std::source_location::current().file_name(),                                      \
                                     std::source_location::current().line(),                                           \
                                     std::source_location::current().function_name(),                                  \
                                     ##__VA_ARGS__)
#ifdef LOG_WARN
#undef LOG_WARN
#endif
#define LOG_WARN(fmt, ...)                                                                                             \
    utils::Logger::getLogger()->warn("[{}:{} {}] " fmt,                                                                \
                                     std::source_location::current().file_name(),                                      \
                                     std::source_location::current().line(),                                           \
                                     std::source_location::current().function_name(),                                  \
                                     ##__VA_ARGS__)
#ifdef LOG_ERROR
#undef LOG_ERROR
#endif
#define LOG_ERROR(fmt, ...)                                                                                            \
    utils::Logger::getLogger()->error("[{}:{} {}] " fmt,                                                               \
                                      std::source_location::current().file_name(),                                     \
                                      std::source_location::current().line(),                                          \
                                      std::source_location::current().function_name(),                                 \
                                      ##__VA_ARGS__)
#ifdef LOG_DEBUG
#undef LOG_DEBUG
#endif
#define LOG_DEBUG(fmt, ...)                                                                                            \
    utils::Logger::getLogger()->debug("[{}:{} {}] " fmt,                                                               \
                                      std::source_location::current().file_name(),                                     \
                                      std::source_location::current().line(),                                          \
                                      std::source_location::current().function_name(),                                 \
                                      ##__VA_ARGS__)
