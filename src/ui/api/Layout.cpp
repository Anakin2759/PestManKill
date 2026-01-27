#include "Layout.hpp"
#include "../singleton/Registry.hpp"
#include "../common/Components.hpp"
#include "../common/Policies.hpp"

namespace ui::layout
{
void MarkLayoutDirty(::entt::entity entity)
{
    entt::entity current = entity;
    while (current != ::entt::null && Registry::Valid(current))
    {
        Registry::EmplaceOrReplace<components::LayoutDirtyTag>(current);
        const auto* hierarchy = Registry::TryGet<components::Hierarchy>(current);
        current = hierarchy != nullptr ? hierarchy->parent : ::entt::null;
    }
}

void SetLayoutDirection(::entt::entity entity, policies::LayoutDirection direction)
{
    if (!Registry::Valid(entity)) return;
    auto& layout = Registry::GetOrEmplace<components::LayoutInfo>(entity);
    layout.direction = static_cast<policies::LayoutDirection>(direction);
    MarkLayoutDirty(entity);
}

void SetLayoutSpacing(::entt::entity entity, float spacing)
{
    if (!Registry::Valid(entity)) return;
    if (auto* layout = Registry::TryGet<components::LayoutInfo>(entity))
    {
        layout->spacing = std::max(0.0F, spacing);
        MarkLayoutDirty(entity);
    }
}

void SetPadding(::entt::entity entity, float left, float top, float right, float bottom)
{
    if (!Registry::Valid(entity)) return;
    auto& padding = Registry::GetOrEmplace<components::Padding>(entity);
    padding.values = Vec4(top, right, bottom, left);
    MarkLayoutDirty(entity);
}

void SetPadding(::entt::entity entity, float padding)
{
    SetPadding(entity, padding, padding, padding, padding);
}

void CenterInParent(::entt::entity entity)
{
    MarkLayoutDirty(entity);
}

} // namespace ui::layout
