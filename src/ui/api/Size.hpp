#pragma once

#include <entt/entt.hpp>
#include "Utils.hpp"
#include "../common/Policies.hpp"
namespace ui::size
{
void SetFixedSize(::entt::entity entity, float width, float height);
void SetSizePolicy(::entt::entity entity, policies::Size policy);
void SetSize(::entt::entity entity, float width, float height);
void SetPosition(::entt::entity entity, float positionX, float positionY);

} // namespace ui::size
