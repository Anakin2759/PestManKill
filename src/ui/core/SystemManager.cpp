/**
 * SystemManager implementation
 */

#include "SystemManager.hpp"
#include "singleton/Registry.hpp"
#include <SDL3/SDL.h>
// 引入所有子系统头文件
#include "../systems/RenderSystem.hpp"
#include "../systems/TweenSystem.hpp"
#include "../systems/InteractionSystem.hpp"
#include "../systems/LayoutSystem.hpp"
#include "../systems/WidgetSystem.hpp" // 保持与 Application.h 中的一致
#include "../systems/ActionSystem.hpp"
namespace ui
{
SystemManager::SystemManager()
{
    m_systems.reserve(SYSTEM_COUNT);
    m_systems.emplace_back(systems::InteractionSystem{});
    m_systems.emplace_back(systems::TweenSystem{});
    m_systems.emplace_back(systems::LayoutSystem{});
    m_systems.emplace_back(systems::RenderSystem{});
    m_systems.emplace_back(systems::WidgetSystem{});
    m_systems.emplace_back(systems::ActionSystem{});
}

SystemManager::~SystemManager()
{
    unregisterAllHandlers();
}

void SystemManager::registerAllHandlers()
{
    for (auto& system : m_systems)
    {
        system->registerHandlers();
    }
}

void SystemManager::unregisterAllHandlers()
{
    for (auto& system : m_systems)
    {
        system->unregisterHandlers();
    }
}

void SystemManager::removeSystem(uint8_t index)
{
    if (index < m_systems.size())
    {
        m_systems.erase(m_systems.begin() + index);
    }
}

void SystemManager::clear()
{
    auto view = Registry::View<components::UiTag>();
    Registry::Destroy(view.begin(), view.end());
}

} // namespace ui
