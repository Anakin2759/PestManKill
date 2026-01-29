#include "Icon.hpp"
using namespace ui::icon;
void SetIcon(entt::entity entity, void* textureId, policies::IconPosition position, float iconSize, float spacing)
{
    if (!Registry::Valid(entity)) return;

    auto& icon = Registry::GetOrEmplace<components::Icon>(entity);
    icon.type = policies::IconType::Texture;
    icon.textureId = textureId;
    icon.fontHandle = nullptr; // 清空字体句柄
    icon.codepoint = 0;
    icon.position = position;
    icon.size = {iconSize, iconSize};
    icon.spacing = spacing;

    // 标记布局需要重新计算
    Registry::EmplaceOrReplace<components::LayoutDirtyTag>(entity);
}

void SetIcon(entt::entity entity,
             void* fontHandle,
             uint32_t codepoint,
             policies::IconPosition position,
             float iconSize,
             float spacing)
{
    if (!Registry::Valid(entity)) return;

    auto& icon = Registry::GetOrEmplace<components::Icon>(entity);
    icon.type = policies::IconType::Font;
    icon.fontHandle = fontHandle;
    icon.codepoint = codepoint;
    icon.textureId = nullptr; // 清空纹理句柄
    icon.position = position;
    icon.size = {iconSize, iconSize};
    icon.spacing = spacing;

    // 标记布局需要重新计算
    Registry::EmplaceOrReplace<components::LayoutDirtyTag>(entity);
}

void RemoveIcon(entt::entity entity)
{
    if (!Registry::Valid(entity)) return;
    if (Registry::AnyOf<components::Icon>(entity))
    {
        Registry::Remove<components::Icon>(entity);
        Registry::EmplaceOrReplace<components::LayoutDirtyTag>(entity);
    }
}