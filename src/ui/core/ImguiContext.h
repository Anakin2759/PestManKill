/**
 * ************************************************************************
 *
 * @file ImguiManager.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-19
 * @version 0.1
 * @brief ImGui 管理类 (集成 SDL3 和 SDL_Renderer3)
    用来初始化和关闭 ImGui 上下文
    提供 ImGui 帧开始和结束的接口
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

namespace ui
{
class ImguiManager
{
public:
    explicit ImguiManager(SDL_Window* window, SDL_Renderer* renderer)
    {
        // 创建 ImGui 上下文
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        // 初始化 ImGui 渲染器后端
        ImGui_ImplSDL3_InitForSDLRenderer(m_context.getWindow(), m_context.getRenderer());
        ImGui_ImplSDLRenderer3_Init(m_context.getRenderer());

        // 设置默认样式
        ImGui::StyleColorsDark();
    }
    ~ImguiManager()
    {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
    ImguiManager(const ImguiManager&) = default;
    ImguiManager& operator=(const ImguiManager&) = default;
    ImguiManager(ImguiManager&&) = default;
    ImguiManager& operator=(ImguiManager&&) = default;
};
} // namespace ui