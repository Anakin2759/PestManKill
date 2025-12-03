
#pragma once

#include <entt/entt.hpp>

namespace events
{
struct NetworkMessageReceived
{
    std::string message;
};
} // namespace events