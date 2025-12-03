#pragma once
#include <entt/entt.hpp>

class Controller
{
public:
    static Controller& getInstance()
    {
        static Controller instance;
        return instance;
    }
    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;
    Controller(Controller&&) = delete;
    Controller& operator=(Controller&&) = delete;

private:
    Controller() = default;
    ~Controller() = default;
    entt::registry m_registry;
};