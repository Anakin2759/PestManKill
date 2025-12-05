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
  基于stb_image实现图片加载
  基于ImGui实现图片渲染

  sdl_render应该
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "Widget.h"
#include "src/client/utils/Logger.h"
#include <cstdint>
#include <string>
#include <string_view>
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

    explicit Image(std::string_view imagePath = "", ScaleMode scaleMode = ScaleMode::FIT)
        : m_imagePath(imagePath), m_scaleMode(scaleMode)
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

    [[nodiscard]] bool isLoaded() const { return m_texture != nullptr; }
    [[nodiscard]] int getWidth() const { return m_width; }
    [[nodiscard]] int getHeight() const { return m_height; }
    [[nodiscard]] const std::string& getImagePath() const { return m_imagePath; }

    // ===================== 加载方法 =====================

    /**
     * @brief 从内存数据加载图片
     * @param data 图片数据指针
     * @param dataSize 数据大小
     * @return 是否加载成功
     */
    bool loadFromMemory(const unsigned char* data, size_t dataSize)
    {
        freeTexture();
        m_imagePath = "[Memory]";

        int channels{0};
        constexpr int REQUIRED_CHANNELS = 4;
        unsigned char* imageData =
            stbi_load_from_memory(data, static_cast<int>(dataSize), &m_width, &m_height, &channels, REQUIRED_CHANNELS);

        if (imageData == nullptr)
        {
            utils::LOG_ERROR("Failed to load image from memory: {}", stbi_failure_reason());
            m_width = 0;
            m_height = 0;
            return false;
        }

        bool success = createTextureFromData(imageData);
        stbi_image_free(imageData);
        return success;
    }

    /**
     * @brief 重新加载图片
     */
    void reload()
    {
        if (!m_imagePath.empty() && m_imagePath != "[Memory]")
        {
            loadImage(m_imagePath);
        }
    }

    /**
     * @brief 清除图片资源
     */
    void clear()
    {
        freeTexture();
        m_imagePath.clear();
        m_width = 0;
        m_height = 0;
    }

    // ===================== 尺寸计算 =====================
    ImVec2 calculateSize() override
    {
        if (isFixedSize())
        {
            return Widget::calculateSize();
        }

        if (m_texture != nullptr && m_width > 0 && m_height > 0)
        {
            return {static_cast<float>(m_width), static_cast<float>(m_height)};
        }

        // 如果图片未加载，返回最小尺寸
        return {getMinWidth(), getMinHeight()};
    }

    void onRender(const ImVec2& position, const ImVec2& size) override
    {
        if (m_texture == nullptr)
        {
            // 绘制占位符：灰色矩形 + X
            renderPlaceholder(position, size);
            return;
        }

        ImVec2 drawSize = calculateDrawSize(size);

        // 居中显示（如果尺寸小于容器）
        ImVec2 drawPos = position;
        if (drawSize.x < size.x)
        {
            drawPos.x += (size.x - drawSize.x) * 0.5F;
        }
        if (drawSize.y < size.y)
        {
            drawPos.y += (size.y - drawSize.y) * 0.5F;
        }

        // 使用 ImGui 绘制 SDL_Texture
        ImGui::SetCursorScreenPos(drawPos);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        ImGui::Image(reinterpret_cast<ImTextureID>(m_texture), drawSize);
    }

private:
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

    bool createTextureFromData(unsigned char* data)
    {
        SDL_Surface* surface = SDL_CreateSurfaceFrom(m_width, m_height, SDL_PIXELFORMAT_RGBA32, data, m_width * 4);
        if (surface == nullptr)
        {
            utils::LOG_ERROR("Failed to create SDL surface: {}", SDL_GetError());
            m_width = 0;
            m_height = 0;
            return false;
        }

        m_texture = SDL_CreateTextureFromSurface(getRenderer(), surface);
        SDL_DestroySurface(surface);

        if (m_texture == nullptr)
        {
            utils::LOG_ERROR("Failed to create SDL texture: {}", SDL_GetError());
            m_width = 0;
            m_height = 0;
            return false;
        }

        return true;
    }

    void renderPlaceholder(const ImVec2& position, const ImVec2& size)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // 灰色背景
        constexpr ImU32 BG_COLOR = IM_COL32(80, 80, 80, 255);
        drawList->AddRectFilled(position, ImVec2(position.x + size.x, position.y + size.y), BG_COLOR);

        // 绘制 X
        constexpr ImU32 X_COLOR = IM_COL32(150, 150, 150, 255);
        constexpr float X_THICKNESS = 2.0F;
        drawList->AddLine(position, ImVec2(position.x + size.x, position.y + size.y), X_COLOR, X_THICKNESS);
        drawList->AddLine(
            ImVec2(position.x + size.x, position.y), ImVec2(position.x, position.y + size.y), X_COLOR, X_THICKNESS);

        // 显示错误文本
        const char* errorText = "Image Load Failed";
        ImVec2 textSize = ImGui::CalcTextSize(errorText);
        ImVec2 textPos(position.x + (size.x - textSize.x) * 0.5F, position.y + (size.y - textSize.y) * 0.5F);
        constexpr ImU32 TEXT_COLOR = IM_COL32(200, 200, 200, 255);
        drawList->AddText(textPos, TEXT_COLOR, errorText);
    }

    bool loadImage(const std::string& path)
    {
        freeTexture();

        // 重置尺寸
        m_width = 0;
        m_height = 0;

        int channels{0};
        constexpr int REQUIRED_CHANNELS = 4;
        unsigned char* data = stbi_load(path.c_str(), &m_width, &m_height, &channels, REQUIRED_CHANNELS);
        if (data == nullptr)
        {
            utils::LOG_ERROR("Failed to load image {}: {}", path, stbi_failure_reason());
            m_width = 0;
            m_height = 0;
            return false;
        }

        bool success = createTextureFromData(data);
        stbi_image_free(data);

        if (success)
        {
            utils::LOG_INFO("Successfully loaded image: {} ({}x{})", path, m_width, m_height);
        }

        return success;
    }

    [[nodiscard]] ImVec2 calculateDrawSize(const ImVec2& containerSize) const
    {
        if (m_texture == nullptr || m_width <= 0 || m_height <= 0)
        {
            return {0.0F, 0.0F};
        }

        const auto WIDTH = static_cast<float>(m_width);
        const auto HEIGHT = static_cast<float>(m_height);

        switch (m_scaleMode)
        {
            case ScaleMode::NONE:
                return {WIDTH, HEIGHT};

            case ScaleMode::FIT:
            {
                const float SCALE = std::min(containerSize.x / WIDTH, containerSize.y / HEIGHT);
                return {WIDTH * SCALE, HEIGHT * SCALE};
            }

            case ScaleMode::FILL:
            {
                const float SCALE = std::max(containerSize.x / WIDTH, containerSize.y / HEIGHT);
                return {WIDTH * SCALE, HEIGHT * SCALE};
            }

            case ScaleMode::STRETCH:
                return containerSize;

            default:
                return {WIDTH, HEIGHT};
        }
    }
};
} // namespace ui
