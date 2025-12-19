/**
 * ************************************************************************
 *
 * @file main.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.2
 * @brief 客户端主程序（集成 UI 和 Net 模块）
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#include <iostream>
#include <memory>
#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

// 客户端核心
#include "GameClient.h"
#include "SceneManager.h"

// 工具
#include <utils.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

class Application
{
public:
    Application(const std::string& title, int width, int height)
        : m_windowTitle(title), m_windowWidth(width), m_windowHeight(height)
    {
    }

    ~Application() { cleanup(); }

    bool initialize()
    {
        // 设置 UTF-8 编码
#if defined(_MSC_VER)
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        std::setlocale(LC_ALL, "C");
        std::locale::global(std::locale("C"));
#endif

        // 初始化 SDL（Application 作为 UI 上下文，负责生命周期）
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        {
            std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
            return false;
        }

        // 创建主窗口（与 UI 上下文解耦：由 createMainWindow 统一负责）
        if (!createMainWindow())
        {
            return false;
        }

        // 初始化 ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        // 设置 ImGui 样式
        ImGui::StyleColorsDark();
        customizeImGuiStyle();

        // 初始化 ImGui SDL3 + Renderer 后端
        ImGui_ImplSDL3_InitForSDLRenderer(m_window, m_renderer);
        ImGui_ImplSDLRenderer3_Init(m_renderer);

        // 创建游戏客户端
        m_gameClient = std::make_unique<GameClient>("127.0.0.1", 9999, "Player1");
        if (!m_gameClient->initialize())
        {
            std::cerr << "Failed to initialize game client" << std::endl;
            return false;
        }

        // 设置场景管理器
        auto& sceneManager = SceneManager::getInstance();
        sceneManager.setClient(m_gameClient.get());
        sceneManager.switchTo("Login"); // 启动时显示登录界面

        return true;
    }

    void run()
    {
        m_running = true;
        auto lastFrameTime = std::chrono::steady_clock::now();

        while (m_running)
        {
            // 计算帧时间
            auto currentTime = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - lastFrameTime).count();
            lastFrameTime = currentTime;

            // 处理事件
            processEvents();

            // 更新逻辑
            update(deltaTime);

            // 渲染
            render();
        }
    }

    void stop() { m_running = false; }

private:
    bool createMainWindow()
    {
        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
        m_window = SDL_CreateWindow(m_windowTitle.c_str(), m_windowWidth, m_windowHeight, window_flags);
        if (!m_window)
        {
            std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
            return false;
        }

        m_renderer = SDL_CreateRenderer(m_window, nullptr);
        if (!m_renderer)
        {
            std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
            return false;
        }

        SDL_SetRenderVSync(m_renderer, 1);
        return true;
    }

    void processEvents()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT)
            {
                stop();
            }

            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(m_window))
            {
                stop();
            }
        }
    }

    void update(float deltaTime)
    {
        // 更新网络和 UI
        if (m_gameClient)
        {
            m_gameClient->updateNetwork();
            m_gameClient->updateUI(deltaTime);
        }

        // 更新场景
        SceneManager::getInstance().update(deltaTime);
    }

    void render()
    {
        // ImGui 新帧
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // 渲染 UI 系统
        if (m_gameClient)
        {
            m_gameClient->renderUI(m_renderer);
        }

        // ImGui 渲染
        ImGui::Render();

        // 清屏
        SDL_SetRenderDrawColor(m_renderer, 20, 20, 25, 255);
        SDL_RenderClear(m_renderer);

        // 渲染 ImGui
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_renderer);

        // 呈现
        SDL_RenderPresent(m_renderer);
    }

    void customizeImGuiStyle()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        // 圆角
        style.WindowRounding = 8.0f;
        style.FrameRounding = 6.0f;
        style.PopupRounding = 6.0f;
        style.ScrollbarRounding = 6.0f;
        style.GrabRounding = 6.0f;
        style.TabRounding = 6.0f;

        // 间距
        style.WindowPadding = ImVec2(12, 12);
        style.FramePadding = ImVec2(10, 6);
        style.ItemSpacing = ImVec2(10, 8);

        // 颜色主题（深色）
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.15f, 0.95f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.2f, 0.25f, 1.0f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.3f, 1.0f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.3f, 0.3f, 0.35f, 1.0f);
        colors[ImGuiCol_Button] = ImVec4(0.25f, 0.5f, 0.7f, 1.0f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.6f, 0.8f, 1.0f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.2f, 0.45f, 0.65f, 1.0f);
    }

    void cleanup()
    {
        if (m_gameClient)
        {
            m_gameClient->stop();
        }

        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();

        if (m_renderer)
        {
            SDL_DestroyRenderer(m_renderer);
        }

        if (m_window)
        {
            SDL_DestroyWindow(m_window);
        }

        SDL_Quit();
    }

private:
    std::string m_windowTitle;
    int m_windowWidth;
    int m_windowHeight;

    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;

    std::unique_ptr<GameClient> m_gameClient;
    bool m_running = false;
};

int main(int argc, char* argv[])
{
    try
    {
        Application app("PestManKill Client", 1280, 720);

        if (!app.initialize())
        {
            std::cerr << "Failed to initialize application" << std::endl;
            return 1;
        }

        app.run();

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}