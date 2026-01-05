/**
 * ************************************************************************
 *
 * @file ImguiContext.h
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
class ImguiContext
{
public:
    explicit ImguiContext(SDL_Window* window, SDL_Renderer* renderer)
    {
        // 创建 ImGui 上下文
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        // 初始化 ImGui 渲染器后端
        ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer3_Init(renderer);

        // 加载中文字体 (使用 Windows 系统字体)
        const char* fontPaths[] = {
            "C:/Windows/Fonts/msyh.ttc",   // 微软雅黑
            "C:/Windows/Fonts/simhei.ttf", // 黑体
            "C:/Windows/Fonts/simsun.ttc", // 宋体
        };

        bool fontLoaded = false;
        for (const char* fontPath : fontPaths)
        {
            if (io.Fonts->AddFontFromFileTTF(fontPath, 18.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull()))
            {
                fontLoaded = true;
                break;
            }
        }

        if (!fontLoaded)
        {
            // 如果没有找到中文字体，使用默认字体
            io.Fonts->AddFontDefault();
        }

        // 设置默认样式
        ImGui::StyleColorsDark();
    }

    ~ImguiContext()
    {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }

    // 禁用拷贝和移动
    ImguiContext(const ImguiContext&) = delete;
    ImguiContext& operator=(const ImguiContext&) = delete;
    ImguiContext(ImguiContext&&) = delete;
    ImguiContext& operator=(ImguiContext&&) = delete;
};
} // namespace ui