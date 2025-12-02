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

    ImVec2 calculateSize() override
    {
        if (isFixedSize())
        {
            return Widget::calculateSize();
        }
        constexpr float BUTTON_PADDING_X = 20.0F;
        constexpr float BUTTON_PADDING_Y = 10.0F;
        ImVec2 textSize = ImGui::CalcTextSize(m_text.c_str());
        return {textSize.x + BUTTON_PADDING_X, textSize.y + BUTTON_PADDING_Y}; // 加上padding
    }

protected:
    void onRender(const ImVec2& position, const ImVec2& size) override
    {
        ImGui::SetCursorPos(position);
        std::string label = m_text + m_uniqueId;
        if (ImGui::Button(label.c_str(), size))
        {
            if (m_onClick)
            {
                m_onClick();
            }
        }
    }

private:
    std::string m_text;
    std::string m_uniqueId;
    std::function<void()> m_onClick;
};
} // namespace ui