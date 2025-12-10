/**
 * ************************************************************************
 *
 * @file ECSApplication.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief ECS化的应用程序类
 *
 * 使用UiSystem管理所有UI元素的ECS版本Application
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <stdexcept>
#include "src/client/model/UiSystem.h"
#include "src/client/utils/utils.h"

namespace ui
{

/**
 * @brief ECS化的应用程序类
 */
class ECSApplication
{
public:
    constexpr static int DEFAULT_WINDOW_WIDTH = 1200;
    constexpr static int DEFAULT_WINDOW_HEIGHT = 800;

    explicit ECSApplication(const char* title, int width = DEFAULT_WINDOW_WIDTH, int height = DEFAULT_WINDOW_HEIGHT)
        : m_windowWidth(width), m_windowHeight(height)
    {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        {
            LOG_ERROR("SDL_Init Error: {}", SDL_GetError());
            throw std::runtime_error(SDL_GetError());
        }

        m_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
        if (m_window == nullptr)
        {
            LOG_ERROR("SDL_CreateWindow Error: {}", SDL_GetError());
            throw std::runtime_error(SDL_GetError());
        }
        LOG_INFO("Window created: {} ({}x{})", title, width, height);

        m_renderer = SDL_CreateRenderer(m_window, nullptr);
        if (m_renderer == nullptr)
        {
            LOG_ERROR("SDL_CreateRenderer Error: {}", SDL_GetError());
            throw std::runtime_error(SDL_GetError());
        }

        initImGui();

        // 创建根UI实体
        m_rootEntity = ui::factory::CreateVBoxLayout();
        auto& rootPos = utils::Registry::getInstance().get<components::Position>(m_rootEntity);
        auto& rootSize = utils::Registry::getInstance().get<components::Size>(m_rootEntity);
        rootPos.x = 0.0F;
        rootPos.y = 0.0F;
        rootSize.width = static_cast<float>(width);
        rootSize.height = static_cast<float>(height);
        rootSize.useFixedSize = true;

        auto& rootBg = utils::Registry::getInstance().emplace<components::Background>(m_rootEntity);
        rootBg.color = ImVec4(20.0F / 255.0F, 25.0F / 255.0F, 33.0F / 255.0F, 1.0F);
        rootBg.enabled = true;
    }

    virtual ~ECSApplication()
    {
        shutdownImGui();
        if (m_renderer != nullptr)
        {
            SDL_DestroyRenderer(m_renderer);
        }
        if (m_window != nullptr)
        {
            SDL_DestroyWindow(m_window);
        }
        SDL_Quit();
    }

    ECSApplication(const ECSApplication&) = delete;
    ECSApplication& operator=(const ECSApplication&) = delete;
    ECSApplication(ECSApplication&&) = delete;
    ECSApplication& operator=(ECSApplication&&) = delete;

    /**
     * @brief 运行主事件循环
     */
    void run()
    {
        bool running = true;
        SDL_Event event;

        Uint64 lastTime = SDL_GetPerformanceCounter();
        const Uint64 frequency = SDL_GetPerformanceFrequency();

        while (running)
        {
            // 计算deltaTime
            Uint64 currentTime = SDL_GetPerformanceCounter();
            float deltaTime = static_cast<float>(currentTime - lastTime) / static_cast<float>(frequency);
            lastTime = currentTime;

            // 事件处理
            while (SDL_PollEvent(&event))
            {
                ImGui_ImplSDL3_ProcessEvent(&event);
                if (event.type == SDL_EVENT_QUIT)
                {
                    running = false;
                }
                else if (event.type == SDL_EVENT_WINDOW_RESIZED)
                {
                    handleResize(event.window.data1, event.window.data2);
                }

                handleEvent(event);
            }

            // 更新
            m_uiSystem.update(deltaTime);

            // 渲染
            m_uiSystem.render(m_renderer);
        }
    }

    /**
     * @brief 获取UI系统
     */
    [[nodiscard]] UiSystem& getUiSystem() { return m_uiSystem; }
    [[nodiscard]] const UiSystem& getUiSystem() const { return m_uiSystem; }

    /**
     * @brief 获取根实体
     */
    [[nodiscard]] entt::entity getRootEntity() const { return m_rootEntity; }

    /**
     * @brief 获取窗口
     */
    [[nodiscard]] SDL_Window* getWindow() { return m_window; }
    [[nodiscard]] const SDL_Window* getWindow() const { return m_window; }

    /**
     * @brief 获取渲染器
     */
    [[nodiscard]] SDL_Renderer* getRenderer() { return m_renderer; }
    [[nodiscard]] const SDL_Renderer* getRenderer() const { return m_renderer; }

protected:
    /**
     * @brief 处理SDL事件（子类可重写）
     */
    virtual void handleEvent(const SDL_Event& /* event */) {}

    /**
     * @brief 设置UI（子类应重写此方法来构建UI）
     */
    virtual void setupUI() {}

private:
    void initImGui()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui::StyleColorsDark();

        ImGui_ImplSDL3_InitForSDLRenderer(m_window, m_renderer);
        ImGui_ImplSDLRenderer3_Init(m_renderer);

        LOG_INFO("ImGui initialized");
    }

    void shutdownImGui()
    {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        LOG_INFO("ImGui shutdown");
    }

    void handleResize(int width, int height)
    {
        m_windowWidth = width;
        m_windowHeight = height;

        // 更新根实体尺寸
        auto& rootSize = utils::Registry::getInstance().get<components::Size>(m_rootEntity);
        rootSize.width = static_cast<float>(width);
        rootSize.height = static_cast<float>(height);
    }

private:
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;

    int m_windowWidth;
    int m_windowHeight;

    UiSystem m_uiSystem;
    entt::entity m_rootEntity = entt::null;
};

} // namespace ui
