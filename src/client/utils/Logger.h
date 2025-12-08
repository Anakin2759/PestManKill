#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
#include <source_location>

namespace utils
{
class Logger
{
public:
    static std::shared_ptr<spdlog::logger>& getLogger()
    {
        static std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_mt("PestManKill");
        return logger;
    }

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

#define LOG_INFO(fmt, ...)                                                                                             \
    utils::Logger::getLogger()->info("[{}:{} {}] " fmt,                                                                \
                                     std::source_location::current().file_name(),                                      \
                                     std::source_location::current().line(),                                           \
                                     std::source_location::current().function_name(),                                  \
                                     ##__VA_ARGS__)

#define LOG_WARN(fmt, ...)                                                                                             \
    utils::Logger::getLogger()->warn("[{}:{} {}] " fmt,                                                                \
                                     std::source_location::current().file_name(),                                      \
                                     std::source_location::current().line(),                                           \
                                     std::source_location::current().function_name(),                                  \
                                     ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...)                                                                                            \
    utils::Logger::getLogger()->error("[{}:{} {}] " fmt,                                                               \
                                      std::source_location::current().file_name(),                                     \
                                      std::source_location::current().line(),                                          \
                                      std::source_location::current().function_name(),                                 \
                                      ##__VA_ARGS__)

#define LOG_DEBUG(fmt, ...)                                                                                            \
    utils::Logger::getLogger()->debug("[{}:{} {}] " fmt,                                                               \
                                      std::source_location::current().file_name(),                                     \
                                      std::source_location::current().line(),                                          \
                                      std::source_location::current().function_name(),                                 \
                                      ##__VA_ARGS__)
