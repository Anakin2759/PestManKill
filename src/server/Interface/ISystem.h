
#pragma once
#include <entt/entt.hpp>
#include <entt/poly/poly.hpp>

struct ISystem : entt::type_list<>
{
    template <typename Base>
    struct type : Base
    {
        void registerEvents() { entt::poly_call<0>(*this); }
        void unregisterEvents() { entt::poly_call<1>(*this); }
    };

    template <typename T>
    using impl = entt::value_list<&T::registerEvents, &T::unregisterEvents>;
};