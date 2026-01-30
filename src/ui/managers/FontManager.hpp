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
#include <memory>
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
    int width = 0;
    int height = 0;
    int xOffset = 0;
    int yOffset = 0;
    int advanceX = 0;
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
    FontManager(FontManager&&) = default;
    FontManager& operator=(FontManager&&) = default;

    /**
     * @brief 从内存加载字体
     * @param fontData 字体数据指针
     * @param dataSize 字体数据大小
     * @param fontSize 字体大小（像素）
     * @return 加载成功返回 true
     */
    bool loadFromMemory(const uint8_t* fontData, size_t dataSize, float fontSize, float oversampleScale = 1.0f)
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
    bool isLoaded() const { return m_loaded; }

    float getOversampleScale() const { return m_oversampleScale; }

    /**
     * @brief 获取字体高度（行高）- 逻辑像素
     */
    int getFontHeight() const
    {
        if (!m_loaded) return 0;
        return static_cast<int>(std::ceil(((m_ascent - m_descent + m_lineGap) * m_scale) / m_oversampleScale));
    }

    /**
     * @brief 获取基线位置 - 逻辑像素
     */
    int getBaseline() const
    {
        if (!m_loaded) return 0;
        return static_cast<int>(std::ceil((m_ascent * m_scale) / m_oversampleScale));
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
            if (outMeasuredLength) *outMeasuredLength = 0;
            return 0;
        }

        int totalWidth = 0;
        size_t bytePos = 0;

        while (bytePos < textLen)
        {
            // 解码 UTF-8 字符
            int codepoint = 0;
            size_t charLen = decodeUTF8(text + bytePos, textLen - bytePos, codepoint);
            if (charLen == 0) break;

            // 获取字形的水平度量
            int advanceWidth = 0;
            int leftSideBearing = 0;
            stbtt_GetCodepointHMetrics(&m_fontInfo, codepoint, &advanceWidth, &leftSideBearing);

            // 逻辑宽度
            int glyphWidth = static_cast<int>(std::ceil((advanceWidth * m_scale) / m_oversampleScale));

            if (maxWidth > 0 && totalWidth + glyphWidth > maxWidth)
            {
                break;
            }

            totalWidth += glyphWidth;
            bytePos += charLen;
        }

        if (outMeasuredLength)
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
        auto it = m_glyphCache.find(codepoint);
        if (it != m_glyphCache.end())
        {
            return it->second;
        }

        // 渲染字形位图
        int x0, y0, x1, y1;
        stbtt_GetCodepointBitmapBox(&m_fontInfo, codepoint, m_scale, m_scale, &x0, &y0, &x1, &y1);

        info.width = x1 - x0;
        info.height = y1 - y0;
        info.xOffset = x0;
        info.yOffset = y0;

        int advanceWidth = 0;
        int leftSideBearing = 0;
        stbtt_GetCodepointHMetrics(&m_fontInfo, codepoint, &advanceWidth, &leftSideBearing);
        info.advanceX = static_cast<int>(advanceWidth * m_scale);

        if (info.width > 0 && info.height > 0)
        {
            info.bitmap.resize(info.width * info.height);
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
    /**
     * @brief 渲染整个文本到 RGBA 位图
     * @param text UTF-8 文本
     * @param color RGBA 颜色 (0-255)
     * @param outWidth 输出位图宽度
     * @param outHeight 输出位图高度
     * @return RGBA 位图数据
     */
    std::vector<uint8_t> renderTextBitmap(
        const std::string& text, uint8_t r, uint8_t g, uint8_t b, uint8_t a, int& outWidth, int& outHeight)
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

        while (bytePos < text.size())
        {
            int codepoint = 0;
            size_t charLen = decodeUTF8(text.c_str() + bytePos, text.size() - bytePos, codepoint);
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
                if (gMinX < minX) minX = gMinX;
                if (gMaxX > maxX) maxX = gMaxX;
                if (gMinY < minY) minY = gMinY;
                if (gMaxY > maxY) maxY = gMaxY;
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
        int fontAscentPixels = static_cast<int>(std::ceil(m_ascent * m_scale));
        int fontHeight = static_cast<int>(std::ceil((m_ascent - m_descent + m_lineGap) * m_scale));

        int baselineY = fontAscentPixels;
        int topOverflow = (minY + baselineY < 0) ? -(minY + baselineY) : 0;
        int bottomOverflow = (maxY + baselineY > fontHeight) ? (maxY + baselineY - fontHeight) : 0;

        outHeight = fontHeight + topOverflow + bottomOverflow;
        int finalBaselineY = baselineY + topOverflow;

        // 创建 RGBA 位图 (初始化为0，全透明)
        result.resize(outWidth * outHeight * 4, 0);

        // 第二遍：渲染字形到位图
        for (size_t i = 0; i < glyphs.size(); ++i)
        {
            const GlyphInfo& glyph = glyphs[i];
            const int xPos = xPositions[i] + glyph.xOffset;
            const int yPos = finalBaselineY + glyph.yOffset;

            for (int y = 0; y < glyph.height; ++y)
            {
                for (int x = 0; x < glyph.width; ++x)
                {
                    const int bitmapX = xPos + x;
                    const int bitmapY = yPos + y;

                    if (bitmapX < 0 || bitmapX >= outWidth || bitmapY < 0 || bitmapY >= outHeight) continue;

                    const int pixelIndex = (bitmapY * outWidth + bitmapX) * 4;
                    const uint8_t srcAlpha = glyph.bitmap[y * glyph.width + x];

                    // 使用 MAX 混合 Alpha，防止重叠字符相互擦除
                    const uint8_t curAlpha = result[pixelIndex + 3];
                    const uint8_t newAlpha = static_cast<uint8_t>(srcAlpha * a / 255);
                    const uint8_t finalAlpha = std::max(curAlpha, newAlpha);

                    // 使用预乘 alpha
                    const float alphaF = finalAlpha / 255.0f;
                    result[pixelIndex + 0] = static_cast<uint8_t>(r * alphaF);
                    result[pixelIndex + 1] = static_cast<uint8_t>(g * alphaF);
                    result[pixelIndex + 2] = static_cast<uint8_t>(b * alphaF);
                    result[pixelIndex + 3] = finalAlpha;
                }
            }
        }

        return result;
    }

private:
    /**
     * @brief 解码 UTF-8 字符
     * @param text UTF-8 字符串指针
     * @param maxBytes 可用的最大字节数
     * @param outCodepoint 输出码点
     * @return 字符占用的字节数，失败返回 0
     */
    static size_t decodeUTF8(const char* text, size_t maxBytes, int& outCodepoint)
    {
        if (maxBytes == 0) return 0;

        const auto byte0 = static_cast<uint8_t>(text[0]);

        // 单字节 ASCII
        if (byte0 < 0x80)
        {
            outCodepoint = byte0;
            return 1;
        }

        // 2 字节
        if ((byte0 & 0xE0) == 0xC0)
        {
            if (maxBytes < 2) return 0;
            outCodepoint = ((byte0 & 0x1F) << 6) | (text[1] & 0x3F);
            return 2;
        }

        // 3 字节
        if ((byte0 & 0xF0) == 0xE0)
        {
            if (maxBytes < 3) return 0;
            outCodepoint = ((byte0 & 0x0F) << 12) | ((text[1] & 0x3F) << 6) | (text[2] & 0x3F);
            return 3;
        }

        // 4 字节
        if ((byte0 & 0xF8) == 0xF0)
        {
            if (maxBytes < 4) return 0;
            outCodepoint =
                ((byte0 & 0x07) << 18) | ((text[1] & 0x3F) << 12) | ((text[2] & 0x3F) << 6) | (text[3] & 0x3F);
            return 4;
        }

        return 0;
    }

private:
    bool m_loaded = false;
    float m_fontSize = 16.0f;
    float m_scale = 1.0f;
    float m_oversampleScale = 1.0f;

    std::vector<uint8_t> m_fontData;
    stbtt_fontinfo m_fontInfo = {};

    int m_ascent = 0;
    int m_descent = 0;
    int m_lineGap = 0;

    // 字形缓存
    std::unordered_map<int, GlyphInfo> m_glyphCache;
};

} // namespace ui::managers
