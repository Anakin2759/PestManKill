/**
 * ************************************************************************
 *
 * @file GraphicsContext.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-23
 * @version 0.1
 * @brief 图形上下文类 - 管理 SDL 窗口和渲染器的生命周期
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <SDL3/SDL.h>
#include <stdexcept>
#include <string>

namespace ui
{

/**
 * @brief 图形上下文类，封装 SDL 窗口和渲染器
 *
 * 采用 RAII 模式，完全管理 SDL 资源的生命周期
 * 构造时创建窗口和渲染器，析构时自动销毁
 */
class GraphicsContext
{
private:
    SDL_Window* m_window = nullptr;
    int m_width = 800;
    int m_height = 600;

public:
    /**
     * @brief 构造函数：创建 SDL 窗口 (不再创建 SDL_Renderer，由 RenderSystem 接管窗口)
     * @param title 窗口标题
     * @param width 窗口宽度
     * @param height 窗口高度
     * @param flags SDL 窗口标志（默认可调整大小）
     */
    explicit GraphicsContext(const char* title = "PestManKill UI",
                             int width = 800,
                             int height = 600,
                             SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE)
        : m_width(width), m_height(height)
    {
        // 创建窗口
        m_window = SDL_CreateWindow(title, width, height, flags);
        if (!m_window)
        {
            throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
        }
    }

    /**
     * @brief 析构函数：自动销毁 SDL 资源
     */
    ~GraphicsContext()
    {
        if (m_window)
        {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }
    }

    // 禁用拷贝，允许移动
    GraphicsContext(const GraphicsContext&) = delete;
    GraphicsContext& operator=(const GraphicsContext&) = delete;

    GraphicsContext(GraphicsContext&& other) noexcept
        : m_window(other.m_window), m_width(other.m_width), m_height(other.m_height)
    {
        other.m_window = nullptr;
    }

    GraphicsContext& operator=(GraphicsContext&& other) noexcept
    {
        if (this != &other)
        {
            if (m_window)
            {
                SDL_DestroyWindow(m_window);
            }

            // 移动资源
            m_window = other.m_window;
            m_width = other.m_width;
            m_height = other.m_height;

            other.m_window = nullptr;
        }
        return *this;
    }

    /**
     * @brief 处理窗口大小调整
     */
    void handleResize(int width, int height)
    {
        m_width = width;
        m_height = height;
    }

    // Getters
    [[nodiscard]] SDL_Window* getWindow() const { return m_window; }
    [[nodiscard]] int getWidth() const { return m_width; }
    [[nodiscard]] int getHeight() const { return m_height; }
};

} // namespace ui
