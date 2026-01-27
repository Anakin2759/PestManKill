#pragma once

#include <entt/entt.hpp>
#include "Utils.hpp"

namespace ui::visibility
{
void SetVisible(::entt::entity entity, bool visible);
void Show(::entt::entity entity);
void Hide(::entt::entity entity);
void SetAlpha(::entt::entity entity, float alpha);
void SetBackgroundColor(::entt::entity entity, const Color& color);
void SetBorderRadius(::entt::entity entity, float radius);
void SetBorderColor(::entt::entity entity, const Color& color);
void SetBorderThickness(::entt::entity entity, float thickness);

} // namespace ui::visibility
