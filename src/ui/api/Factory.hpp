/**
 * API header for UI factory functions
 */
#pragma once
#include <entt/entt.hpp>
#include <string>
#include <string_view>
#include "../core/Application.hpp"
#ifdef CreateWindow
#undef CreateWindow
#endif
#ifdef CreateWindowA
#undef CreateWindowA
#endif
#ifdef CreateDialog
#undef CreateDialog
#endif
#ifdef CreateDialogA
#undef CreateDialogA
#endif

namespace ui::factory
{

Application CreateApplication(int argc, char* argv[]);

entt::entity CreateBaseWidget(std::string_view alias = "");

void CreateFadeInAnimation(entt::entity entity, float duration);

entt::entity CreateButton(const std::string& content, std::string_view alias = "");
entt::entity CreateLabel(const std::string& content, std::string_view alias = "");
entt::entity CreateTextEdit(const std::string& placeholder = "", bool multiline = false, std::string_view alias = "");
entt::entity
    CreateImage(void* textureId, float defaultWidth = 50.0F, float defaultHeight = 50.0F, std::string_view alias = "");
entt::entity CreateArrow(const Vec2& start, const Vec2& end, std::string_view alias = "");
entt::entity CreateSpacer(int stretchFactor = 1, std::string_view alias = "");
entt::entity CreateSpacer(float width, float height, std::string_view alias = "");
entt::entity CreateDialog(std::string_view title, std::string_view alias = "");
entt::entity CreateScrollArea(std::string_view alias = "");
entt::entity CreateWindow(std::string_view title, std::string_view alias = "");
entt::entity CreateVBoxLayout(std::string_view alias = "");
entt::entity CreateHBoxLayout(std::string_view alias = "");
entt::entity
    CreateLineEdit(std::string_view initialText = "", std::string_view placeholder = "", std::string_view alias = "");
entt::entity CreateTextBrowser(std::string_view initialText = "",
                               std::string_view placeholder = "",
                               std::string_view alias = "");
entt::entity CreateCheckBox(const std::string& label, bool checked = false, std::string_view alias = "");

// Icon API
void SetIcon(entt::entity entity,
             const std::string& textureId,
             policies::IconPosition position = policies::IconPosition::Left,
             float iconSize = 16.0F,
             float spacing = 4.0F);

void SetIcon(entt::entity entity,
             const std::string& fontName,
             uint32_t codepoint,
             policies::IconPosition position = policies::IconPosition::Left,
             float iconSize = 16.0F,
             float spacing = 4.0F);

void RemoveIcon(entt::entity entity);

} // namespace ui::factory
