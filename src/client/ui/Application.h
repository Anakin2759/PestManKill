/**
 * ************************************************************************
 *
 * @file Application.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-11-27
 * @version 0.1
 * @brief 主应用类定义
    IMGUI+SDL3实现的UI应用框架
    模拟 Qt 的应用类 QApplication
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include "Layout.h"
#include "Widget.h"
#include <stdexcept>
#include "../utils/Logger.h"
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
namespace ui
{
// 主应用类
class Application : public Widget
{
public:
    constexpr static int DEFAULT_WINDOW_WIDTH = 1200;
    constexpr static int DEFAULT_WINDOW_HEIGHT = 800;
    constexpr static int DEFAULT_SPACING = 10;
    constexpr static int DEFAULT_MARGINS = 10;
    Application(const char* title, int width, int height)
    {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        {
            utils::LOG_ERROR("SDL_Init Error: {}", SDL_GetError());
            throw std::runtime_error(SDL_GetError());
        }

        m_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
        if (m_window == nullptr)
        {
            utils::LOG_ERROR("SDL_CreateWindow Error: {}", SDL_GetError());
            throw std::runtime_error(SDL_GetError());
        }
        utils::LOG_INFO("Window created: {} ({}x{})", title, width, height);
        setRenderer(SDL_CreateRenderer(m_window, nullptr));
        if (getRenderer() == nullptr)
        {
            throw std::runtime_error(SDL_GetError());
        }

        // 设置默认背景色为深灰色
        setBackgroundColor(ImVec4(20.0F / 255.0F, 25.0F / 255.0F, 33.0F / 255.0F, 1.0F));
        setBackgroundEnabled(true);

        initImGui();
    }

    ~Application() override
    {
        shutdownImGui();
        if (getRenderer() != nullptr)
        {
            SDL_DestroyRenderer(getRenderer());
        }
        if (m_window != nullptr)
        {
            SDL_DestroyWindow(m_window);
        }
        SDL_Quit();
    }

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    void run()
    {
        setupUI(); // 在运行前初始化UI
        m_running = true;
        while (m_running)
        {
            processEvents();
            render();
        }
    }

    void stop() { m_running = false; }

protected:
    virtual void setupUI() = 0;

    void setRootLayout(std::shared_ptr<Layout> layout) { m_rootLayout = std::move(layout); }

    // 实现 Widget 的 onRender 方法
    void onRender(const ImVec2& /*position*/, const ImVec2& /*size*/) override
    {
        // Application 的渲染逻辑在 onGui 中处理
        // 这里不需要做任何事情，因为 Application 使用自己的渲染流程
    }

    virtual void onGui()
    {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

        ImGui::Begin("MainWindow",
                     nullptr,
                     static_cast<ImGuiWindowFlags>(static_cast<unsigned int>(ImGuiWindowFlags_NoTitleBar) |
                                                   static_cast<unsigned int>(ImGuiWindowFlags_NoResize) |
                                                   static_cast<unsigned int>(ImGuiWindowFlags_NoMove) |
                                                   static_cast<unsigned int>(ImGuiWindowFlags_NoScrollbar) |
                                                   static_cast<unsigned int>(ImGuiWindowFlags_NoScrollWithMouse) |
                                                   static_cast<unsigned int>(ImGuiWindowFlags_NoCollapse)));

        if (m_rootLayout)
        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            m_rootLayout->render(ImVec2(0, 0), avail);
        }

        ImGui::End();
    }

private:
    SDL_Window* m_window = nullptr;

    bool m_running = true;
    std::shared_ptr<Layout> m_rootLayout;

    void processEvents()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT)
            {
                m_running = false;
            }
        }
    }

    void render()
    {
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        onGui();

        ImGui::Render();

        // 使用 Widget 的背景色设置
        const ImVec4& bgColor = getBackgroundColor();
        SDL_SetRenderDrawColor(getRenderer(),
                               static_cast<Uint8>(bgColor.x * 255.0F),
                               static_cast<Uint8>(bgColor.y * 255.0F),
                               static_cast<Uint8>(bgColor.z * 255.0F),
                               static_cast<Uint8>(bgColor.w * 255.0F));
        SDL_RenderClear(getRenderer());
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), getRenderer());
        SDL_RenderPresent(getRenderer());
    }

    void initImGui()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGuiIO& imGuiIo = ImGui::GetIO();
        ImFontConfig fontCfg;
        fontCfg.OversampleH = 3;
        fontCfg.OversampleV = 1;
        fontCfg.PixelSnapH = true;

        // 加载中文字体，并合并常用符号
        static constexpr std::array<ImWchar, 12> FONT_RANGES = {
            0x0020,
            0x00FF, // 基本拉丁字母
            0x2000,
            0x206F, // 常规标点
            0x2600,
            0x26FF, // 杂项符号（包含扑克牌花色）
            0x4E00,
            0x9FFF, // CJK统一汉字
            0,
        };

        imGuiIo.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/msyh.ttc",
                                          // NOLINTNEXTLINE
                                          18.0F,
                                          &fontCfg,
                                          FONT_RANGES.data());

        ImGui_ImplSDL3_InitForSDLRenderer(m_window, getRenderer());
        ImGui_ImplSDLRenderer3_Init(getRenderer());
    }

    static void shutdownImGui()
    {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
};
} // namespace ui
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)