
#pragma once

#include <entt/entt.hpp>

namespace events
{
struct NetworkMessageReceived
{
    std::string message;
};

struct Login
{
};

struct Logout
{
};

struct CreateRoom
{
};

struct JoinRoom
{
};

} // namespace events