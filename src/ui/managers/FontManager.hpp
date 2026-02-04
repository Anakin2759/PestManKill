/**
 * ************************************************************************
 *
 * @file FontManager.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-30
 * @version 0.1
 * @brief STB TrueType 字体管理器
 *
 * 使用 stb_truetype 替代 SDL_ttf 进行字体渲染
    - 默认加载 ui/assets/fonts/ 下的 TTF 字体文件
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <stb_truetype.h>

#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <cstdint>
#include <cmath>

namespace ui::managers
{

/**
 * @brief 字形信息结构
 */
struct GlyphInfo
{
    int width = 0;               // 位图宽度
    int height = 0;              // 位图宽高
    int xOffset = 0;             // 水平偏移
    int yOffset = 0;             // 垂直偏移
    int advanceX = 0;            // 水平前进量
    std::vector<uint8_t> bitmap; // 灰度位图数据
};

/**
 * @brief 字体管理器，封装 stb_truetype 功能
 */
class FontManager
{
public:
    FontManager() = default;
    ~FontManager() = default;

    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;
    FontManager(FontManager&&) noexcept = default;
    FontManager& operator=(FontManager&&) = default;

    /**
     * @brief 从内存加载字体
     * @param fontData 字体数据指针
     * @param dataSize 字体数据大小
     * @param fontSize 字体大小（像素）
     * @return 加载成功返回 true
     */
    bool loadFromMemory(const uint8_t* fontData, size_t dataSize, float fontSize, float oversampleScale = 1.0F)
    {
        m_fontSize = fontSize;
        m_oversampleScale = oversampleScale;

        // 复制字体数据，因为 stbtt_fontinfo 需要持久的内存
        m_fontData.resize(dataSize);
        std::memcpy(m_fontData.data(), fontData, dataSize);

        // 初始化字体
        const int offset = stbtt_GetFontOffsetForIndex(m_fontData.data(), 0);
        if (stbtt_InitFont(&m_fontInfo, m_fontData.data(), offset) == 0)
        {
            return false;
        }

        // 计算缩放因子
        m_scale = stbtt_ScaleForPixelHeight(&m_fontInfo, fontSize * m_oversampleScale);

        // 获取字体度量
        stbtt_GetFontVMetrics(&m_fontInfo, &m_ascent, &m_descent, &m_lineGap);

        m_loaded = true;
        return true;
    }

    /**
     * @brief 检查字体是否已加载
     */
    [[nodiscard]] bool isLoaded() const { return m_loaded; }

    [[nodiscard]] float getOversampleScale() const { return m_oversampleScale; }

    /**
     * @brief 获取字体高度（行高）- 逻辑像素
     */
    [[nodiscard]] int getFontHeight() const
    {
        if (!m_loaded) return 0;
        return static_cast<int>(
            std::ceil((static_cast<float>(m_ascent - m_descent + m_lineGap) * m_scale) / m_oversampleScale));
    }

    /**
     * @brief 获取基线位置 - 逻辑像素
     */
    [[nodiscard]] int getBaseline() const
    {
        if (!m_loaded) return 0;
        return static_cast<int>(std::ceil((static_cast<float>(m_ascent) * m_scale) / m_oversampleScale));
    }

    /**
     * @brief 测量 UTF-8 文本的宽度
     * @param text UTF-8 编码的文本
     * @param maxWidth 最大宽度（像素），超过此宽度则停止测量
     * @param outMeasuredLength 输出实际测量的字节长度
     * @return 文本的像素宽度
     */
    int measureString(const char* text, size_t textLen, int maxWidth, size_t* outMeasuredLength)
    {
        if (!m_loaded || text == nullptr || textLen == 0)
        {
            if (outMeasuredLength != nullptr) *outMeasuredLength = 0;
            return 0;
        }

        int totalWidth = 0;
        size_t bytePos = 0;
        std::string_view view(text, textLen);

        while (bytePos < textLen)
        {
            // 解码 UTF-8 字符
            int codepoint = 0;
            size_t charLen = decodeUTF8(view.substr(bytePos), codepoint);
            if (charLen == 0) break;

            // 获取字形的水平度量
            int advanceWidth = 0;
            int leftSideBearing = 0;
            stbtt_GetCodepointHMetrics(&m_fontInfo, codepoint, &advanceWidth, &leftSideBearing);

            // 逻辑宽度
            int glyphWidth =
                static_cast<int>(std::ceil((static_cast<float>(advanceWidth) * m_scale) / m_oversampleScale));

            if (maxWidth > 0 && totalWidth + glyphWidth > maxWidth)
            {
                break;
            }

            totalWidth += glyphWidth;
            bytePos += charLen;
        }

        if (outMeasuredLength != nullptr)
        {
            *outMeasuredLength = bytePos;
        }

        return totalWidth;
    }

    /**
     * @brief 测量文本宽度（简化版本）
     */
    int measureTextWidth(const std::string& text)
    {
        size_t measuredLen = 0;
        return measureString(text.c_str(), text.size(), 0, &measuredLen);
    }

    /**
     * @brief 渲染单个字形到灰度位图
     * @param codepoint Unicode 码点
     * @return 字形信息
     */
    GlyphInfo renderGlyph(int codepoint)
    {
        GlyphInfo info;

        if (!m_loaded) return info;

        // 检查缓存
        auto iterator = m_glyphCache.find(codepoint);
        if (iterator != m_glyphCache.end())
        {
            return iterator->second;
        }

        // 渲染字形位图
        int fx0{};
        int fy0{};
        int fx1{};
        int fy1{};
        stbtt_GetCodepointBitmapBox(&m_fontInfo, codepoint, m_scale, m_scale, &fx0, &fy0, &fx1, &fy1);

        info.width = fx1 - fx0;
        info.height = fy1 - fy0;
        info.xOffset = fx0;
        info.yOffset = fy0;

        int advanceWidth = 0;
        int leftSideBearing = 0;
        stbtt_GetCodepointHMetrics(&m_fontInfo, codepoint, &advanceWidth, &leftSideBearing);
        info.advanceX = static_cast<int>(static_cast<float>(advanceWidth) * m_scale);

        if (info.width > 0 && info.height > 0)
        {
            info.bitmap.resize(static_cast<size_t>(info.width) * static_cast<size_t>(info.height));
            stbtt_MakeCodepointBitmap(
                &m_fontInfo, info.bitmap.data(), info.width, info.height, info.width, m_scale, m_scale, codepoint);
        }

        // 缓存字形
        m_glyphCache[codepoint] = info;

        return info;
    }

    /**
     * @brief 渲染整个文本到 RGBA 位图
     * @param text UTF-8 文本
     * @param color RGBA 颜色 (0-255)
     * @param outWidth 输出位图宽度
     * @param outHeight 输出位图高度
     * @return RGBA 位图数据
     */
    std::vector<uint8_t> renderTextBitmap(
        const std::string& text, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha, int& outWidth, int& outHeight)
    {
        std::vector<uint8_t> result;

        if (!m_loaded || text.empty())
        {
            outWidth = outHeight = 0;
            return result;
        }

        // 第一遍：计算总宽度和收集字形
        std::vector<GlyphInfo> glyphs;
        std::vector<int> xPositions;

        int cursorX = 0;
        int minX = 0;
        int maxX = 0;
        int minY = 0;
        int maxY = 0;
        bool first = true;

        size_t bytePos = 0;
        std::string_view view(text);

        while (bytePos < text.size())
        {
            int codepoint = 0;
            size_t charLen = decodeUTF8(view.substr(bytePos), codepoint);
            if (charLen == 0) break;

            GlyphInfo glyph = renderGlyph(codepoint);
            glyphs.push_back(glyph);
            xPositions.push_back(cursorX);

            int gMinX = cursorX + glyph.xOffset;
            int gMaxX = gMinX + glyph.width;
            int gMinY = glyph.yOffset;
            int gMaxY = gMinY + glyph.height;

            if (first)
            {
                minX = gMinX;
                maxX = gMaxX;
                minY = gMinY;
                maxY = gMaxY;
                first = false;
            }
            else
            {
                minX = std::min(minX, gMinX);
                maxX = std::max(maxX, gMaxX);
                minY = std::min(minY, gMinY);
                maxY = std::max(maxY, gMaxY);
            }

            cursorX += glyph.advanceX;
            bytePos += charLen;
        }

        if (glyphs.empty())
        {
            outWidth = outHeight = 0;
            return result;
        }

        // 计算最终尺寸
        // 宽度：覆盖所有像素，并且至少包含逻辑宽度(cursorX)，始于0
        outWidth = std::max(cursorX, maxX);

        // 高度：确保容纳所有像素，同时保持基线对其
        int fontAscentPixels = static_cast<int>(std::ceil(static_cast<float>(m_ascent) * m_scale));
        int fontHeight = static_cast<int>(std::ceil(static_cast<float>(m_ascent - m_descent + m_lineGap) * m_scale));

        int baselineY = fontAscentPixels;
        int topOverflow = (minY + baselineY < 0) ? -(minY + baselineY) : 0;
        int bottomOverflow = (maxY + baselineY > fontHeight) ? (maxY + baselineY - fontHeight) : 0;

        outHeight = fontHeight + topOverflow + bottomOverflow;
        int finalBaselineY = baselineY + topOverflow;

        // 创建 RGBA 位图 (初始化为颜色值，Alpha 为 0)
        // 使用直通 Alpha (Straight Alpha) 而非预乘 Alpha，以避免两次混合导致的变暗问题
        // 同时填充 RGB 通道以避免 bilinear filtering 时的边缘黑边
        result.resize(static_cast<size_t>(outWidth) * static_cast<size_t>(outHeight) * 4);
        for (int i = 0; i < outWidth * outHeight; ++i)
        {
            result[(i * 4) + 0] = red;
            result[(i * 4) + 1] = green;
            result[(i * 4) + 2] = blue;
            result[(i * 4) + 3] = 0;
        }

        // 第二遍：渲染字形到位图
        for (size_t i = 0; i < glyphs.size(); ++i)
        {
            const GlyphInfo& glyph = glyphs[i];
            const int xPos = xPositions[i] + glyph.xOffset;
            const int yPos = finalBaselineY + glyph.yOffset;

            for (int yOffset = 0; yOffset < glyph.height; ++yOffset)
            {
                for (int xOffset = 0; xOffset < glyph.width; ++xOffset)
                {
                    const int bitmapX = xPos + xOffset;
                    const int bitmapY = yPos + yOffset;

                    if (bitmapX < 0 || bitmapX >= outWidth || bitmapY < 0 || bitmapY >= outHeight) continue;

                    const int pixelIndex = (bitmapY * outWidth + bitmapX) * 4;
                    const uint8_t srcAlpha = glyph.bitmap[(yOffset * glyph.width) + xOffset];

                    // 使用 MAX 混合 Alpha，防止重叠字符相互擦除
                    const uint8_t curAlpha = result[pixelIndex + 3];
                    const auto newAlpha = static_cast<uint8_t>(srcAlpha * alpha / 255);
                    const uint8_t finalAlpha = std::max(curAlpha, newAlpha);

                    // 仅更新 Alpha通道 (Straight Alpha)
                    result[pixelIndex + 3] = finalAlpha;
                }
            }
        }

        return result;
    }

private:
    /**
     * @brief 解码 UTF-8 字符
     * @param text UTF-8 字符串视图
     * @param outCodepoint 输出码点
     * @return 字符占用的字节数，失败返回 0
     */
    static size_t decodeUTF8(std::string_view text, int& outCodepoint)
    {
        if (text.empty()) return 0;

        const auto byte0 = static_cast<uint8_t>(text[0]);

        // 单字节 ASCII
        if (byte0 < 0x80)
        {
            outCodepoint = byte0;
            return 1;
        }

        // 2 字节
        if ((byte0 & 0xE0U) == 0xC0)
        {
            if (text.size() < 2) return 0;
            outCodepoint = static_cast<int>(((byte0 & 0x1FU) << 6U) | (static_cast<uint8_t>(text[1]) & 0x3FU));
            return 2;
        }

        // 3 字节
        if ((byte0 & 0xF0U) == 0xE0)
        {
            if (text.size() < 3) return 0;
            outCodepoint = static_cast<int>(((byte0 & 0x0FU) << 12U) | ((static_cast<uint8_t>(text[1]) & 0x3FU) << 6U) |
                                            (static_cast<uint8_t>(text[2]) & 0x3FU));
            return 3;
        }

        // 4 字节
        if ((byte0 & 0xF8U) == 0xF0)
        {
            if (text.size() < 4) return 0;
            outCodepoint = static_cast<int>(
                ((byte0 & 0x07U) << 18U) | ((static_cast<uint8_t>(text[1]) & 0x3FU) << 12U) |
                ((static_cast<uint8_t>(text[2]) & 0x3FU) << 6U) | (static_cast<uint8_t>(text[3]) & 0x3FU));
            return 4;
        }

        return 0;
    }

    bool m_loaded = false;
    float m_fontSize = 16.0F;
    float m_scale = 1.0F;
    float m_oversampleScale = 1.0F;

    std::vector<uint8_t> m_fontData;
    stbtt_fontinfo m_fontInfo = {};

    int m_ascent = 0;
    int m_descent = 0;
    int m_lineGap = 0;

    // 字形缓存
    std::unordered_map<int, GlyphInfo> m_glyphCache;
};

} // namespace ui::managers
