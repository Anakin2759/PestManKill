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

    void setText(std::string_view text) { m_text = text; }

    [[nodiscard]] std::string getText() const { return m_text; }

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