#include "Hierarchy.hpp"
#include "../singleton/Registry.hpp"
#include "../common/Components.hpp"
#include "Layout.hpp"
namespace ui::hierarchy
{
void RemoveChild(::entt::entity parent, ::entt::entity child)
{
    if (!Registry::Valid(parent) || !Registry::Valid(child)) return;

    auto* parentHierarchy = Registry::TryGet<components::Hierarchy>(parent);
    auto* childHierarchy = Registry::TryGet<components::Hierarchy>(child);

    if (parentHierarchy != nullptr && childHierarchy != nullptr && childHierarchy->parent == parent)
    {
        auto& children = parentHierarchy->children;
        std::erase(children, child);
        childHierarchy->parent = ::entt::null;
        layout::MarkLayoutDirty(parent);
    }
}

void AddChild(::entt::entity parent, ::entt::entity child)
{
    if (!Registry::Valid(parent) || !Registry::Valid(child)) return;

    auto& childHierarchy = Registry::GetOrEmplace<components::Hierarchy>(child);
    if (childHierarchy.parent != ::entt::null && childHierarchy.parent != parent)
    {
        RemoveChild(childHierarchy.parent, child);
    }
    childHierarchy.parent = parent;

    auto& parentHierarchy = Registry::GetOrEmplace<components::Hierarchy>(parent);
    auto& children = parentHierarchy.children;
    bool alreadyChild = false;
    for (auto c : children)
    {
        if (c == child)
        {
            alreadyChild = true;
            break;
        }
    }
    if (!alreadyChild)
    {
        children.push_back(child);
    }

    layout::MarkLayoutDirty(child);
}

} // namespace ui::hierarchy
