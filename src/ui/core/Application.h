/**
 * ************************************************************************
 *
 * @file Application.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-11 (Updated)
 * @version 0.2
 * @brief ui上下文管理类
 *
 * 负责主循环、输入事件处理、图形上下文管理以及驱动所有ECS系统。
    存在一个单例指针，方便全局访问。
    不是根实体，也不管理根实体
    只负责驱动UiSystem和处理SDL/ImGui集成
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
#include <stdexcept>
#include <chrono>

#include "src/client/model/UiSystem.h"
#include "src/client/systems/WindowSystem.h" // 假设存在一个处理窗口尺寸变化的系统
#include "src/client/utils/utils.h"          // 包含 Registry, Dispatcher, GraphicsContext

namespace ui
{
class Application
{
private:
    // 核心 ECS 系统封装
    SystemManager m_systems;

    //ImGui 上下文管理
    

    // ECS 根实体，代表整个屏幕/应用区域
    entt::entity m_rootEntity = entt::null;

    bool running = true;
    SDL_Event event{};

    // 阻止拷贝和移动（通常 Application 是单例或独占资源）
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

public:
    /**
     * @brief 构造函数：初始化所有外部和内部资源
     * 假设 GraphicsContext 已经实例化且可通过 utils::getInstance() 获取
     */
    explicit Application()

        m_lastTime(Clock::now())
    {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
        {
            throw std::runtime_error("SDL_Init failed.");
        }

        // 1. ImGui 初始化 (依赖于 m_context 提供的 window 和 renderer)
        initImGui();

        // 2. 初始化 ECS 根实体
        setupRootEntity();

        // 3. 设置用户UI (留给子类实现或独立函数调用)
        setupUI();
    }

    virtual ~Application()
    {
        shutdownImGui();
        SDL_Quit();
        // GraphicsContext 析构函数应自动清理 Window/Renderer
    }

    /**
     * @brief 应用主循环
     */
    void run()
    {
        while (running)
        {
            // --- 1. 时间步长计算 ---
            TimePoint now = Clock::now();
            float deltaTime = std::chrono::duration<float>(now - m_lastTime).count();
            m_lastTime = now;

            // --- 2. 事件处理 ---
            while (SDL_PollEvent(&event))
            {
                ImGui_ImplSDL3_ProcessEvent(&event); // 转发给 ImGui

                if (event.type == SDL_EVENT_QUIT)
                {
                    running = false;
                }

                if (event.type == SDL_EVENT_WINDOW_RESIZED)
                {
                    // 通知 GraphicsContext 处理内部缩放
                    m_context.handleResize(event.window.data1, event.window.data2);

                    // 通知 ECS WindowSystem 更新根实体尺寸
                    m_windowSystem.onResize(m_rootEntity, m_context.getWidth(), m_context.getHeight());
                }

                handleEvent(event); // 用户自定义事件处理
            }

            // --- 3. ECS 更新流程 (System::update) ---
            m_uiSystem.update(deltaTime);

            // --- 4. 渲染流程 (System::render) ---

            // 4a. 启动 ImGui 新帧
            ImGui_ImplSDLRenderer3_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            // 4b. 渲染应用UI (由 ECS RenderSystem 处理)
            SDL_SetRenderDrawColor(m_context.getRenderer(), 0, 0, 0, 255);
            SDL_RenderClear(m_context.getRenderer());

            m_uiSystem.render(m_context.getRenderer());

            // 4c. 渲染 ImGui UI
            ImGui::Render();
            ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_context.getRenderer());

            // 4d. 交换缓冲区
            SDL_RenderPresent(m_context.getRenderer());
        }
    }

    // Getters
    [[nodiscard]] UiSystem& getUiSystem() { return m_uiSystem; }
    [[nodiscard]] entt::entity getRootEntity() const { return m_rootEntity; }

protected:
    /**
     * @brief 抽象函数：留给子类或外部调用者实现具体的UI结构。
     */
    virtual void setupUI() = 0;

    /**
     * @brief 抽象函数：留给子类处理未被ECS系统消耗的原始SDL事件。
     */
    virtual void handleEvent(const SDL_Event& event) {}

private:
    void initImGui()
    {
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

    void shutdownImGui()
    {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
};

} // namespace ui