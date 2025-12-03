#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>
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
// NOLINTBEGIN

template <typename... Args>
inline void LOG_INFO(fmt::format_string<Args...> fmt, Args&&... args)
{
    Logger::getLogger()->info(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void LOG_WARN(fmt::format_string<Args...> fmt, Args&&... args)
{
    Logger::getLogger()->warn(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void LOG_ERROR(fmt::format_string<Args...> fmt, Args&&... args)
{
    Logger::getLogger()->error(fmt, std::forward<Args>(args)...);
}
} // namespace utils
// NOLINTEND