#include "Utils.hpp"
#include <cstdint>

namespace ui::utils
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
void MarkRenderDirty(::entt::entity entity)
{
    Registry::EmplaceOrReplace<components::RenderDirtyTag>(entity);
}

bool HasAlignment(policies::Alignment value, policies::Alignment flag)
{
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
}

void SetWindowFlag(::entt::entity entity, policies::WindowFlag flag)
{
    if (!Registry::Valid(entity)) return;
    auto& windowComp = Registry::GetOrEmplace<components::Window>(entity);

    windowComp.flags |= flag;
}

} // namespace ui::utils
