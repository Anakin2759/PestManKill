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

namespace ui
{
class Image : public Widget
{
public:
    enum class ScaleMode : uint8_t
    {
        NONE,   // 不缩放，使用原始尺寸
        FIT,    // 等比例缩放以适应组件尺寸
        FILL,   // 填充组件尺寸，可能会裁剪
        STRETCH // 拉伸以填充组件尺寸，可能会变形
    };

    explicit Image(const std::string& imagePath = "", ScaleMode scaleMode = ScaleMode::FIT);
    ~Image() override;
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&&) = delete;
    Image& operator=(Image&&) = delete;

    void setImagePath(const std::string& imagePath);
    [[nodiscard]] const std::string& getImagePath() const { return m_imagePath; }
    void setScaleMode(ScaleMode scaleMode) { m_scaleMode = scaleMode; }
    [[nodiscard]] ScaleMode getScaleMode() const { return m_scaleMode; }

protected:
    void onRender(const ImVec2& position, const ImVec2& size) override;

private:
    void loadImage(const std::string& imagePath);
    void freeImage();
    std::string m_imagePath;
    ScaleMode m_scaleMode = ScaleMode::FIT;
    ImTextureID m_texture = 0;
    int m_imageWidth = 0;
    int m_imageHeight = 0;
};
} // namespace ui