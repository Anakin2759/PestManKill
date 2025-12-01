/**
 * ************************************************************************
 *
 * @file ALabel.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-11-27
 * @version 0.1
 * @brief
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
// 标签组件
class Label : public Widget
{
public:
    explicit Label(std::string text) : m_text(std::move(text)) {}

    ImVec2 calculateSize() override
    {
        if (isFixedSize())
        {
            return {getMinWidth(), getMinHeight()};
        }
        return ImGui::CalcTextSize(m_text.c_str());
    }

protected:
    void onRender(const ImVec2& position, const ImVec2& /*size*/) override
    {
        ImGui::SetCursorPos(position);
        ImGui::TextUnformatted(m_text.c_str());
    }

private:
    std::string m_text;
};
} // namespace ui