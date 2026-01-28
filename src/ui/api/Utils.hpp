#pragma once

#include <entt/entt.hpp>
#include "../singleton/Registry.hpp"
#include "../common/Types.hpp"
#include "../common/Policies.hpp"
namespace ui::utils
{
bool HasAlignment(policies::Alignment value, policies::Alignment flag);
void SetWindowFlag(::entt::entity entity, policies::WindowFlag flag);

void MarkLayoutDirty(::entt::entity entity);
void MarkRenderDirty(::entt::entity entity);

} // namespace ui::utils
