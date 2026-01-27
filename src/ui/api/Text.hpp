#pragma once

#include <entt/entt.hpp>
#include <string>
#include "Utils.hpp"
#include "../common/Policies.hpp"
namespace ui::text
{
void SetButtonText(::entt::entity entity, const std::string& content);
void SetButtonEnabled(::entt::entity entity, bool enabled);
void SetLabelText(::entt::entity entity, const std::string& content);
void SetTextContent(::entt::entity entity, const std::string& content);
void SetTextWordWrap(::entt::entity entity, policies::TextWrap mode);
void SetTextAlignment(::entt::entity entity, policies::Alignment alignment);
void SetTextColor(::entt::entity entity, const Color& color);
std::string GetTextEditContent(::entt::entity entity);
void SetTextEditContent(::entt::entity entity, const std::string& content);
void SetPasswordMode(::entt::entity entity, policies::TextFlag enabled);
void SetClickCallback(::entt::entity entity, components::on_event<> callback);

} // namespace ui::text
