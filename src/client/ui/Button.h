/**
 * ************************************************************************
 *
 * @file AButton.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-01
 * @version 0.1
 * @brief 按钮组件定义
  模拟 Qt 的按钮组件 QPushButton
  所有按钮均继承自该类

    - 提供设置按钮文本的方法
    - 提供点击事件回调注册方法
    - 提供尺寸计算方法（基于文本大小加上内边距）
    - 提供渲染方法（基于 ImGui 实现按钮绘制和点击处理）
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "Widget.h"
#include <string>
#include <functional>
// 按钮组件
namespace ui
{

class Button : public Widget
{
public:
    explicit Button(std::string text, std::function<void()> onClick = nullptr)
        : m_text(std::move(text)), m_onClick(std::move(onClick))
    {
        static size_t idCounter = 0;
        m_uniqueId = "##Button_" + std::to_string(idCounter++);
    }

    // ===================== 文本管理 =====================
    void setText(const std::string& text) { m_text = text; }
    [[nodiscard]] const std::string& getText() const { return m_text; }

    // ===================== 状态管理 =====================
    void setEnabled(bool enabled) { m_enabled = enabled; }
    [[nodiscard]] bool isEnabled() const { return m_enabled; }

    // ===================== 样式设置 =====================
    void setButtonColor(const ImVec4& color)
    {
        m_buttonColor = color;
        m_useCustomColor = true;
    }

    void setHoverColor(const ImVec4& color)
    {
        m_hoverColor = color;
        m_useCustomColor = true;
    }

    void setActiveColor(const ImVec4& color)
    {
        m_activeColor = color;
        m_useCustomColor = true;
    }

    void resetColors() { m_useCustomColor = false; }

    // ===================== 提示文本 =====================
    void setTooltip(const std::string& tooltip) { m_tooltip = tooltip; }
    [[nodiscard]] const std::string& getTooltip() const { return m_tooltip; }

    // ===================== 回调管理 =====================
    void setOnClick(std::function<void()> onClick) { m_onClick = std::move(onClick); }

    ImVec2 calculateSize() override
    {
        if (isFixedSize())
        {
            return Widget::calculateSize();
        }
        constexpr float BUTTON_PADDING_X = 20.0F;
        constexpr float BUTTON_PADDING_Y = 10.0F;
        ImVec2 textSize = ImGui::CalcTextSize(m_text.c_str());
        return {textSize.x + BUTTON_PADDING_X, textSize.y + BUTTON_PADDING_Y};
    }

protected:
    void onRender(const ImVec2& position, const ImVec2& size) override
    {
        ImGui::SetCursorPos(position);

        // 应用自定义颜色
        int colorsPushed = 0;
        if (m_useCustomColor)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, m_buttonColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, m_hoverColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, m_activeColor);
            colorsPushed = 3;
        }

        // 禁用状态样式
        if (!m_enabled)
        {
            constexpr float DISABLED_ALPHA = 0.5F;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * DISABLED_ALPHA);
        }

        std::string label = m_text + m_uniqueId;
        bool clicked = ImGui::Button(label.c_str(), size);

        // 处理点击
        if (clicked && m_enabled && m_onClick)
        {
            m_onClick();
        }

        // 显示提示文本
        if (!m_tooltip.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        {
            ImGui::SetTooltip("%s", m_tooltip.c_str());
        }

        // 恢复样式
        if (!m_enabled)
        {
            ImGui::PopStyleVar();
        }

        if (colorsPushed > 0)
        {
            ImGui::PopStyleColor(colorsPushed);
        }
    }

private:
    std::string m_text;
    std::string m_uniqueId;
    std::string m_tooltip;
    std::function<void()> m_onClick;

    bool m_enabled = true;
    bool m_useCustomColor = false;

    ImVec4 m_buttonColor{0.26F, 0.59F, 0.98F, 0.40F}; // 默认按钮颜色
    ImVec4 m_hoverColor{0.26F, 0.59F, 0.98F, 1.00F};  // 默认悬停颜色
    ImVec4 m_activeColor{0.06F, 0.53F, 0.98F, 1.00F}; // 默认激活颜色
};
} // namespace ui