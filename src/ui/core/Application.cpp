/**
 * Implementation for Application
 */

#include "Application.hpp"
#include "singleton/Dispatcher.hpp"
#include "singleton/Logger.hpp"
#include <SDL3/SDL.h>
static constexpr int DEFAULT_WIDTH = 800;
static constexpr int DEFAULT_HEIGHT = 600;
static constexpr int FRAME_DELAY_MS = 16;     // ~60 FPS
static constexpr int RENDER_DELAY_MS = 0;     // ~60 FPS
static constexpr int MAX_FRAME_TIME_MS = 250; // 防止卡顿时长时间更新
static constexpr int LOOP_DELAY_MS = 1;       // 主循环延迟，防止100% CPU占用
namespace ui
{
Application::Application(int argc, char* argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    Logger::info("SDL 初始化成功");

    m_systems.registerAllHandlers();

    m_scheduler.attach<ui::QueuedTaskChain>();
    m_scheduler.attach<ui::EventTaskChain>(FRAME_DELAY_MS);
    m_scheduler.attach<ui::InputTaskChain>(FRAME_DELAY_MS);
    m_scheduler.attach<ui::RenderTaskChain>(RENDER_DELAY_MS);

    m_eventLoop.registerDefaultHandler(
        [this]()
        {
            auto now = std::chrono::steady_clock::now();

            // 2. 计算差值（毫秒级，保留浮点精度）
            std::chrono::duration<float, std::milli> diff = now - m_lastUpdateTime;
            float dtMs = diff.count();

            // 3. 更新最后一次时间点
            m_lastUpdateTime = now;

            // 4. 安全保护：防止 dt 过大（如断点调试后）
            if (dtMs > static_cast<float>(MAX_FRAME_TIME_MS))
            {
                dtMs = static_cast<float>(MAX_FRAME_TIME_MS);
            }

            // 5. 将毫秒差值填入调度器
            m_scheduler.update(dtMs);
            SDL_Delay(LOOP_DELAY_MS);
        });

    Dispatcher::Sink<ui::events::QuitRequested>().connect<&Application::onQuitRequested>(*this);
}

Application::~Application()
{
    m_systems.unregisterAllHandlers();
    SDL_Quit();
}

void Application::onQuitRequested([[maybe_unused]] ui::events::QuitRequested& /*event*/)
{
    m_eventLoop.quit();
}

void Application::exec()
{
    m_eventLoop.exec();
}

} // namespace ui
