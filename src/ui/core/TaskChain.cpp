/**
 * TaskChain implementations
 */

#include "TaskChain.hpp"
#include "singleton/Dispatcher.hpp"
#include "singleton/Logger.hpp"
#include <SDL3/SDL.h>
namespace ui
{
InputTaskChain::InputTaskChain(const allocator_type& alloc, delta_type delay)
    : entt::process{alloc}, m_remainingTime(delay), m_delayTime(delay)
{
}

void InputTaskChain::update(const delta_type delta, [[maybe_unused]] void* data)
{
    
}

void InputTaskChain::succeeded()
{
    Logger::info("[input task] InputTask succeeded");
}
void InputTaskChain::failed()
{
    Logger::info("[input task] InputTask failed");
}
void InputTaskChain::aborted()
{
    Logger::info("[input task] InputTask aborted");
}

RenderTaskChain::RenderTaskChain(const allocator_type& alloc, delta_type delay)
    : entt::process{alloc}, m_remainingTime(delay), m_delayTime(delay)
{
}

void RenderTaskChain::update(const delta_type delta, [[maybe_unused]] void* data)
{
    static bool firstCall = true;
    if (firstCall)
    {
        Logger::info("[RenderTaskChain] First update call - delta: {}, remainingTime: {}", delta, m_remainingTime);
        firstCall = false;
    }

    if (m_remainingTime > delta)
    {
        m_remainingTime -= delta;
        return;
    }

    static int renderCount = 0;
    if (renderCount < 3)
    {
        Logger::info("[RenderTaskChain] Triggering render #{}", renderCount++);
    }

    m_remainingTime = m_delayTime;

    Dispatcher::Trigger<ui::events::UpdateLayout>(ui::events::UpdateLayout{});
    Dispatcher::Trigger<ui::events::UpdateRendering>(ui::events::UpdateRendering{});
}

void RenderTaskChain::succeeded()
{
    Logger::info("[render task] RenderTask succeeded");
}
void RenderTaskChain::failed()
{
    Logger::info("[render task] RenderTask failed");
}
void RenderTaskChain::aborted()
{
    Logger::info("[render task] RenderTask aborted");
}

EventTaskChain::EventTaskChain(const allocator_type& alloc, delta_type delay)
    : entt::process{alloc}, m_remainingTime(delay), m_delayTime(delay)
{
}

void EventTaskChain::update(const delta_type delta, [[maybe_unused]] void* data)
{
    if (m_remainingTime > delta)
    {
        m_remainingTime -= delta;
        return;
    }
    m_remainingTime = m_delayTime;
    Dispatcher::Update<events::QueuedTask>();
    Dispatcher::Update();
}




} // namespace ui
