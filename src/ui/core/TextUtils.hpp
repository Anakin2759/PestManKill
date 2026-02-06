/**
 * ************************************************************************
 *
 * @file TextUtils.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-30
 * @version 0.1
 * @brief 文本处理工具函数
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <string>
#include <vector>
#include <functional>
#include "../common/Components.hpp"

namespace ui::utils
{
/**
 * @brief 换行处理单个段落
 */
template <typename MeasureFunc>
inline std::vector<std::string>
    WrapParagraph(const std::string& paragraph, int maxWidth, policies::TextWrap wrapMode, MeasureFunc&& measureFunc)
{
    std::vector<std::string> lines;

    if (paragraph.empty())
    {
        lines.push_back("");
        return lines;
    }

    std::string currentLine;
    std::string word;

    for (auto c : paragraph)
    {
        if (c == ' ' || c == '\t')
        {
            // 空格或制表符
            if (!word.empty())
            {
                std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
                float width = measureFunc(testLine);

                if (width > maxWidth && !currentLine.empty())
                {
                    // 需要换行
                    lines.push_back(currentLine);
                    currentLine = word;
                }
                else
                {
                    currentLine = testLine;
                }
                word.clear();
            }

            // 如果是Word模式且当前行非空，添加空格
            if (wrapMode == policies::TextWrap::Word && !currentLine.empty())
            {
                currentLine += c;
            }
        }
        else
        {
            word += c;
        }
    }

    // 处理最后一个单词
    if (!word.empty())
    {
        std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
        float width = measureFunc(testLine);

        if (width > static_cast<float>(maxWidth) && !currentLine.empty())
        {
            lines.push_back(currentLine);
            currentLine = word;
        }
        else
        {
            currentLine = testLine;
        }
    }

    if (!currentLine.empty())
    {
        lines.push_back(currentLine);
    }

    return lines;
}

/**
 * @brief 文本换行处理
 */
template <typename MeasureFunc>
inline std::vector<std::string>
    WrapTextLines(const std::string& text, int maxWidth, policies::TextWrap wrapMode, MeasureFunc&& measureFunc)
{
    std::vector<std::string> lines;

    if (wrapMode == policies::TextWrap::NONE || maxWidth <= 0)
    {
        lines.push_back(text);
        return lines;
    }

    // 按换行符分割
    std::string currentParagraph;
    for (char c : text)
    {
        if (c == '\n')
        {
            if (!currentParagraph.empty())
            {
                // 处理当前段落
                auto wrappedLines = WrapParagraph(currentParagraph, maxWidth, wrapMode, measureFunc);
                lines.insert(lines.end(), wrappedLines.begin(), wrappedLines.end());
                currentParagraph.clear();
            }
            lines.push_back(""); // 空行
        }
        else
        {
            currentParagraph += c;
        }
    }

    // 处理最后一段
    if (!currentParagraph.empty())
    {
        auto wrappedLines = WrapParagraph(currentParagraph, maxWidth, wrapMode, measureFunc);
        lines.insert(lines.end(), wrappedLines.begin(), wrappedLines.end());
    }

    return lines;
}

/**
 * @brief 获取能够显示在指定宽度内的文本尾部
 */
template <typename MeasureFunc>
inline std::string GetTailThatFits(const std::string& text, int maxWidth, MeasureFunc&& measureFunc, float& outWidth)
{
    outWidth = 0.0f;

    if (text.empty() || maxWidth <= 0)
    {
        return "";
    }

    // 从尾部开始截取
    for (size_t i = 0; i < text.size(); ++i)
    {
        std::string substr = text.substr(text.size() - i - 1);
        float width = measureFunc(substr);

        if (width <= maxWidth)
        {
            outWidth = width;
            if (i == text.size() - 1)
            {
                return substr; // 全部都能显示
            }
        }
        else
        {
            // 超出宽度，返回上一次的结果
            if (i > 0)
            {
                return text.substr(text.size() - i);
            }
            return "";
        }
    }

    outWidth = measureFunc(text);
    return text;
}

} // namespace ui::utils
