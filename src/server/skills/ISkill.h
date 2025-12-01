#include <entt/entt.hpp>
#include <entt/poly/poly.hpp>
#include "src/context/GameContext.h"

struct ISkill : entt::type_list<>
{
    template <typename Base>
    struct type : Base
    {
        void onEvent(int pt) const { return this->template invoke<0>(*this, pt); }
    };

    template <typename Type>
    using impl = entt::value_list<&Type::onEvent>;
};