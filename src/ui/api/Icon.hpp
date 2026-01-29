#pragma once
#include <entt/entt.hpp>
#include <string>
#include "../common/Policies.hpp"
#include "../common/Components.hpp"

namespace ui::icon
{

void SetIcon(entt::entity entity,
                    void* textureId,
                    policies::IconPosition position = policies::IconPosition::Left,
                    float iconSize = 16.0F,
                    float spacing = 4.0F);


void SetIcon(entt::entity entity,
                 void* fontHandle,
                 uint32_t codepoint,
                 policies::IconPosition position = policies::IconPosition::Left,
                 float iconSize = 16.0F,
                 float spacing = 4.0F);


void RemoveIcon(entt::entity entity);
} // namespace ui::icon