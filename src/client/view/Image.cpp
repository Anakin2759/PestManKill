/**
 * ************************************************************************
 *
 * @file Image.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-01
 * @version 0.1
 * @brief 图片组件实现
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#include "Image.h"
#include <spdlog/spdlog.h>
#include <algorithm>

// 在一个 .cpp 文件中定义 stb_image 实现
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace ui
{

Image::Image(const std::string& imagePath, ScaleMode scaleMode) : m_imagePath(imagePath), m_scaleMode(scaleMode)
{
    if (!imagePath.empty())
    {
        loadImage(imagePath);
    }
}

Image::~Image()
{
    freeImage();
}

void Image::setImagePath(const std::string& imagePath)
{
    if (m_imagePath == imagePath)
    {
        return;
    }

    freeImage();
    m_imagePath = imagePath;

    if (!imagePath.empty())
    {
        loadImage(imagePath);
    }
}

void Image::loadImage(const std::string& imagePath)
{
    // 释放旧纹理
    freeImage();

    // 使用 stb_image 加载图片
    int channels = 0;
    unsigned char* imageData = stbi_load(imagePath.c_str(), &m_imageWidth, &m_imageHeight, &channels, STBI_rgb_alpha);

    if (!imageData)
    {
        spdlog::error("Failed to load image: {}", imagePath);
        m_imageWidth = 0;
        m_imageHeight = 0;
        return;
    }

    // 获取 ImGui 的 SDL 渲染器
    ImGuiIO& io = ImGui::GetIO();
    SDL_Renderer* renderer = static_cast<SDL_Renderer*>(io.BackendRendererUserData);

    if (!renderer)
    {
        spdlog::error("Failed to get SDL_Renderer from ImGui");
        stbi_image_free(imageData);
        return;
    }

    // 创建 SDL 纹理并直接存储为 ImTextureID
    SDL_Texture* sdlTexture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, m_imageWidth, m_imageHeight);

    if (!sdlTexture)
    {
        spdlog::error("Failed to create texture: {}", SDL_GetError());
        stbi_image_free(imageData);
        m_imageWidth = 0;
        m_imageHeight = 0;
        return;
    }

    // 更新纹理数据
    const int pitch = m_imageWidth * 4; // RGBA，每像素 4 字节
    if (SDL_UpdateTexture(sdlTexture, nullptr, imageData, pitch) != 0)
    {
        spdlog::error("Failed to update texture: {}", SDL_GetError());
        SDL_DestroyTexture(sdlTexture);
        stbi_image_free(imageData);
        m_imageWidth = 0;
        m_imageHeight = 0;
        return;
    }

    // 存储为 ImTextureID (在新版 ImGui 中 ImTextureID 是 ImU64 类型)
    m_texture = reinterpret_cast<ImTextureID>(sdlTexture);

    // 释放 stb_image 分配的内存
    stbi_image_free(imageData);

    spdlog::info("Loaded image: {} ({}x{})", imagePath, m_imageWidth, m_imageHeight);
}

void Image::freeImage()
{
    if (m_texture != 0)
    {
        SDL_DestroyTexture(reinterpret_cast<SDL_Texture*>(m_texture));
        m_texture = 0;
    }
    m_imageWidth = 0;
    m_imageHeight = 0;
}

void Image::onRender(const ImVec2& position, const ImVec2& size)
{
    if (!m_texture || m_imageWidth <= 0 || m_imageHeight <= 0)
    {
        // 如果没有纹理，显示占位符
        if (ImDrawList* drawList = ImGui::GetWindowDrawList())
        {
            ImVec2 topLeft = ImGui::GetCursorScreenPos();
            ImVec2 bottomRight = ImVec2(topLeft.x + size.x, topLeft.y + size.y);

            // 绘制灰色背景
            drawList->AddRectFilled(topLeft, bottomRight, IM_COL32(128, 128, 128, 255));

            // 绘制 X 标记
            drawList->AddLine(topLeft, bottomRight, IM_COL32(255, 255, 255, 255), 2.0f);
            drawList->AddLine(
                ImVec2(topLeft.x, bottomRight.y), ImVec2(bottomRight.x, topLeft.y), IM_COL32(255, 255, 255, 255), 2.0f);
        }
        return;
    }

    // 计算渲染尺寸和位置
    ImVec2 renderSize = size;
    ImVec2 uvMin = ImVec2(0.0f, 0.0f);
    ImVec2 uvMax = ImVec2(1.0f, 1.0f);

    const float imageAspect = static_cast<float>(m_imageWidth) / static_cast<float>(m_imageHeight);
    const float containerAspect = size.x / size.y;

    switch (m_scaleMode)
    {
        case ScaleMode::NONE:
            // 使用原始尺寸
            renderSize.x = static_cast<float>(m_imageWidth);
            renderSize.y = static_cast<float>(m_imageHeight);
            break;

        case ScaleMode::FIT:
            // 等比例缩放以适应容器
            if (imageAspect > containerAspect)
            {
                // 图片更宽，以宽度为准
                renderSize.x = size.x;
                renderSize.y = size.x / imageAspect;
            }
            else
            {
                // 图片更高，以高度为准
                renderSize.y = size.y;
                renderSize.x = size.y * imageAspect;
            }
            break;

        case ScaleMode::FILL:
            // 填充容器，可能裁剪
            if (imageAspect > containerAspect)
            {
                // 图片更宽，以高度为准，裁剪宽度
                renderSize = size;
                const float uvWidth = containerAspect / imageAspect;
                uvMin.x = (1.0f - uvWidth) * 0.5f;
                uvMax.x = (1.0f + uvWidth) * 0.5f;
            }
            else
            {
                // 图片更高，以宽度为准，裁剪高度
                renderSize = size;
                const float uvHeight = imageAspect / containerAspect;
                uvMin.y = (1.0f - uvHeight) * 0.5f;
                uvMax.y = (1.0f + uvHeight) * 0.5f;
            }
            break;

        case ScaleMode::STRETCH:
            // 拉伸以填充容器
            renderSize = size;
            break;
    }

    // 居中显示（如果尺寸小于容器）
    ImVec2 renderPos = position;
    if (renderSize.x < size.x)
    {
        renderPos.x += (size.x - renderSize.x) * 0.5f;
    }
    if (renderSize.y < size.y)
    {
        renderPos.y += (size.y - renderSize.y) * 0.5f;
    }

    // 使用 ImGui 绘制图像
    ImGui::SetCursorPos(renderPos);
    ImGui::Image(m_texture, renderSize, uvMin, uvMax);

    // 渲染子组件
    renderChildren(position);
}

} // namespace ui
