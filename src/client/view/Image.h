/**
 * ************************************************************************
 *
 * @file Image.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-01
 * @version 0.1
 * @brief  图片组件定义
  模拟 Qt 的图片组件 QLabel（带图片功能）
  支持加载和显示图片
  支持设置图片缩放模式
  支持设置图片路径动态更新图片
  基于SDL2_image实现图片加载
  基于ImGui实现图片渲染
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "Widget.h"
#include <cstdint>
#include <string>
#include <SDL3/SDL.h>
#include <stb_image.h>
#include <imgui.h>

namespace ui
{
class Image : public Widget
{
public:
    enum class ScaleMode : uint8_t
    {
        NONE,   // 原始尺寸
        FIT,    // 等比例适应组件
        FILL,   // 等比例填充容器，可能裁剪
        STRETCH // 拉伸填充容器，可能变形
    };

    explicit Image(SDL_Renderer* renderer, const std::string& imagePath = "", ScaleMode scaleMode = ScaleMode::FIT)
        : m_renderer(renderer), m_imagePath(imagePath), m_scaleMode(scaleMode)
    {
        if (!m_imagePath.empty())
        {
            loadImage(m_imagePath);
        }
    }

    ~Image() override { freeTexture(); }

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&&) = delete;
    Image& operator=(Image&&) = delete;

    void setImagePath(const std::string& path)
    {
        if (m_imagePath != path)
        {
            m_imagePath = path;
            loadImage(m_imagePath);
        }
    }

    void setScaleMode(ScaleMode mode) { m_scaleMode = mode; }

    void onRender(const ImVec2& position, const ImVec2& size) override
    {
        if (m_texture == nullptr)
        {
            return;
        }

        ImVec2 drawSize = calculateDrawSize(size);

        // 使用 ImGui 绘制 SDL_Texture
        ImGui::SetCursorScreenPos(position);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        ImGui::Image(reinterpret_cast<ImTextureID>(m_texture), drawSize);
    }

private:
    SDL_Renderer* m_renderer;
    std::string m_imagePath;
    ScaleMode m_scaleMode;
    SDL_Texture* m_texture{nullptr};
    int m_width{0};
    int m_height{0};

    void freeTexture()
    {
        if (m_texture != nullptr)
        {
            SDL_DestroyTexture(m_texture);
            m_texture = nullptr;
        }
    }

    bool loadImage(const std::string& path)
    {
        freeTexture();

        int channels{0};
        constexpr int REQUIRED_CHANNELS = 4;
        unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &channels, REQUIRED_CHANNELS);
        if (data == nullptr)
        {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION, "Failed to load image %s: %s", path.c_str(), stbi_failure_reason());
            return false;
        }

        // SDL3: 使用 SDL_CreateSurfaceFrom 替代 SDL_CreateRGBSurfaceWithFormatFrom
        SDL_Surface* surface =
            SDL_CreateSurfaceFrom(m_width, m_height, SDL_PIXELFORMAT_RGBA32, data, m_width * REQUIRED_CHANNELS);
        if (surface == nullptr)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create SDL surface: %s", SDL_GetError());
            stbi_image_free(data);
            return false;
        }

        m_texture = SDL_CreateTextureFromSurface(m_renderer, surface);
        SDL_DestroySurface(surface); // SDL3: SDL_FreeSurface 重命名为 SDL_DestroySurface
        stbi_image_free(data);

        if (m_texture == nullptr)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create SDL texture: %s", SDL_GetError());
            return false;
        }

        return true;
    }

    ImVec2 calculateDrawSize(const ImVec2& containerSize)
    {
        if (m_texture == nullptr)
        {
            return ImVec2(0, 0);
        }

        auto width = static_cast<float>(m_width);
        auto height = static_cast<float>(m_height);

        switch (m_scaleMode)
        {
            case ScaleMode::NONE:
                return ImVec2(width, height);

            case ScaleMode::FIT:
            {
                float scale = std::min(containerSize.x / width, containerSize.y / height);
                return ImVec2(width * scale, height * scale);
            }

            case ScaleMode::FILL:
            {
                float scale = std::max(containerSize.x / width, containerSize.y / height);
                return ImVec2(width * scale, height * scale);
            }

            case ScaleMode::STRETCH:
                return containerSize;

            default:
                return ImVec2(width, height);
        }
    }
};
} // namespace ui
