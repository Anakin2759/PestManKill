#include "Text.hpp"
#include "../singleton/Registry.hpp"
#include "../common/Components.hpp"
#include "../common/Tags.hpp"

#include "Layout.hpp"
namespace ui::text
{
void SetText(::entt::entity entity, const std::string& content)
{
    if (!Registry::Valid(entity)) return;
    if (Registry::AnyOf<components::Text>(entity))
    {
        auto& text = Registry::GetOrEmplace<components::Text>(entity);
        text.content = content;
        utils::MarkLayoutDirty(entity);
    }
}

void SetButtonEnabled(::entt::entity entity, bool enabled)
{
    if (!Registry::Valid(entity)) return;
    if (enabled)
    {
        Registry::Remove<components::DisabledTag>(entity);
    }
    else
    {
        Registry::EmplaceOrReplace<components::DisabledTag>(entity);
    }
}

void SetTextContent(::entt::entity entity, const std::string& content)
{
    if (!Registry::Valid(entity)) return;
    auto& text = Registry::GetOrEmplace<components::Text>(entity);
    text.content = content;
    utils::MarkLayoutDirty(entity);
}

void SetTextWordWrap(::entt::entity entity, policies::TextWrap mode)
{
    if (!Registry::Valid(entity)) return;
    auto& text = Registry::GetOrEmplace<components::Text>(entity);
    text.wordWrap = mode;
    utils::MarkLayoutDirty(entity);
}

void SetTextAlignment(::entt::entity entity, policies::Alignment alignment)
{
    if (!Registry::Valid(entity)) return;
    auto& text = Registry::GetOrEmplace<components::Text>(entity);
    text.alignment = alignment;
    utils::MarkLayoutDirty(entity);
}

void SetTextColor(::entt::entity entity, const Color& color)
{
    if (!Registry::Valid(entity)) return;
    if (auto* textComp = Registry::TryGet<components::Text>(entity)) textComp->color = color;
    if (auto* textEdit = Registry::TryGet<components::TextEdit>(entity)) textEdit->textColor = color;
}

std::string GetTextEditContent(::entt::entity entity)
{
    if (!Registry::Valid(entity)) return "";
    if (auto* textEdit = Registry::TryGet<components::TextEdit>(entity)) return textEdit->buffer;
    return "";
}

void SetTextEditContent(::entt::entity entity, const std::string& content)
{
    if (!Registry::Valid(entity)) return;
    if (auto* textEdit = Registry::TryGet<components::TextEdit>(entity))
    {
        textEdit->buffer = content;
        // Ensure cursor is within bounds to prevent "invalid string position" exception
        if (textEdit->cursorPosition > textEdit->buffer.size())
        {
            textEdit->cursorPosition = textEdit->buffer.size();
        }
        // Reset selection on content change to ensure validity
        textEdit->hasSelection = false;
        textEdit->selectionStart = 0;
        textEdit->selectionEnd = 0;
    }
}

void SetPasswordMode(::entt::entity entity, policies::TextFlag enabled)
{
    if (!Registry::Valid(entity)) return;
    if (auto* textEdit = Registry::TryGet<components::TextEdit>(entity)) textEdit->inputMode |= enabled;
}

void SetClickCallback(::entt::entity entity, components::on_event<> callback)
{
    if (!Registry::Valid(entity)) return;
    auto& clickable = Registry::GetOrEmplace<components::Clickable>(entity);
    clickable.onClick = std::move(callback);
    clickable.enabled = policies::Feature::Enabled;
}

} // namespace ui::text
