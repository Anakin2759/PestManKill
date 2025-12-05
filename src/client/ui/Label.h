/**
 * ************************************************************************
 *
 * @file ALabel.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-11-27
 * @version 0.1
 * @brief 标签组件类定义
    一个简单的文本显示组件
    用于显示静态文本标签
    支持基本的文本属性设置（字体、颜色等）

    - 提供setText和getText方法修改和获取文本内容
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "Widget.h"
#include <string>
namespace ui
{
// 文本对齐方式
enum class TextAlignment : uint8_t
{
    LEFT,
    CENTER,
    RIGHT
};

// 标签组件
class Label : public Widget
{
public:
    explicit Label(std::string text) : m_text(std::move(text)) {}

    // ===================== 文本管理 =====================
    void setText(std::string_view text) { m_text = text; }
    [[nodiscard]] std::string getText() const { return m_text; }

    // ===================== 样式设置 =====================
    void setTextColor(const ImVec4& color)
    {
        m_textColor = color;
        m_useCustomColor = true;
    }

    void resetTextColor() { m_useCustomColor = false; }

    void setAlignment(TextAlignment alignment) { m_alignment = alignment; }
    [[nodiscard]] TextAlignment getAlignment() const { return m_alignment; }

    void setFontScale(float scale) { m_fontScale = std::max(0.1F, scale); }
    [[nodiscard]] float getFontScale() const { return m_fontScale; }

    void setWordWrap(bool wrap) { m_wordWrap = wrap; }
    [[nodiscard]] bool isWordWrap() const { return m_wordWrap; }

    // ===================== 尺寸计算 =====================
    ImVec2 calculateSize() override
    {
        if (isFixedSize())
        {
            return Widget::calculateSize();
        }

        // 保存当前字体缩放
        float oldScale = ImGui::GetFont()->Scale;
        ImGui::GetFont()->Scale = m_fontScale;
        ImGui::PushFont(ImGui::GetFont());

        ImVec2 textSize;
        if (m_wordWrap && getMaxWidth() < FLT_MAX)
        {
            // 如果启用换行且有最大宽度限制
            textSize = ImGui::CalcTextSize(m_text.c_str(), nullptr, false, getMaxWidth());
        }
        else
        {
            textSize = ImGui::CalcTextSize(m_text.c_str());
        }

        ImGui::PopFont();
        ImGui::GetFont()->Scale = oldScale;

        return textSize;
    }

protected:
    void onRender(const ImVec2& position, const ImVec2& size) override
    {
        // 应用字体缩放
        float oldScale = ImGui::GetFont()->Scale;
        ImGui::GetFont()->Scale = m_fontScale;
        ImGui::PushFont(ImGui::GetFont());

        // 应用自定义颜色
        if (m_useCustomColor)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, m_textColor);
        }

        ImVec2 textSize = ImGui::CalcTextSize(m_text.c_str(), nullptr, m_wordWrap, m_wordWrap ? size.x : 0.0F);
        ImVec2 renderPos = position;

        // 应用对齐方式
        switch (m_alignment)
        {
            case TextAlignment::CENTER:
                renderPos.x += (size.x - textSize.x) * 0.5F;
                break;
            case TextAlignment::RIGHT:
                renderPos.x += size.x - textSize.x;
                break;
            case TextAlignment::LEFT:
            default:
                break;
        }

        ImGui::SetCursorPos(renderPos);

        if (m_wordWrap)
        {
            ImGui::PushTextWrapPos(renderPos.x + size.x);
            ImGui::TextUnformatted(m_text.c_str());
            ImGui::PopTextWrapPos();
        }
        else
        {
            ImGui::TextUnformatted(m_text.c_str());
        }

        // 恢复样式
        if (m_useCustomColor)
        {
            ImGui::PopStyleColor();
        }

        ImGui::PopFont();
        ImGui::GetFont()->Scale = oldScale;
    }

private:
    std::string m_text;
    ImVec4 m_textColor{1.0F, 1.0F, 1.0F, 1.0F};
    TextAlignment m_alignment = TextAlignment::LEFT;
    float m_fontScale = 1.0F;
    bool m_wordWrap = false;
    bool m_useCustomColor = false;
};
} // namespace ui