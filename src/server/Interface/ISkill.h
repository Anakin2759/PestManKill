#include <entt/entt.hpp>
#include <entt/poly/poly.hpp>
#include "src/context/GameContext.h"

struct ISkill : entt::type_list<>
{
    template <typename Base>
    struct type : Base
    {
        void onUse() const { return this->template invoke<0>(*this); }
        absl::flat_hash_map<entt::entity, bool> filterTargets(entt::entity pt) const
        {
            return this->template invoke<1>(*this, pt);
        }
    };

    template <typename Type>
    using impl = entt::value_list<&Type::onUse, &Type::filterTargets>;
};