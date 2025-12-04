/**
 * ************************************************************************
 *
 * @file TextEdit.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-04
 * @version 0.1
 * @brief 文本编辑控件类定义
    一个多行文本编辑框组件
    支持文本输入和显示
    支持基本的文本编辑功能（光标移动、文本选择、复制粘贴等）
    支持垂直滚动条

    - 提供设置和获取文本内容的方法
    - 提供设置只读模式的方法
    - 提供文本变更回调机制
    - 提供设置最大文本长度的方法
    - 提供设置是否自动换行的方法
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
#include <imgui.h>

namespace ui
{

/**
 * @brief 多行文本编辑控件
 * 基于 ImGui::InputTextMultiline 实现
 * 支持只读模式和可编辑模式
 * 支持文本变更回调
 */
class TextEdit : public Widget
{
public:
    TextEdit()
    {
        static size_t idCounter = 0;
        m_uniqueId = "##TextEdit_" + std::to_string(idCounter++);

        // 默认设置最小尺寸
        constexpr float DEFAULT_MIN_WIDTH = 200.0F;  // NOLINT
        constexpr float DEFAULT_MIN_HEIGHT = 100.0F; // NOLINT
        setMinimumSize(DEFAULT_MIN_WIDTH, DEFAULT_MIN_HEIGHT);

        // 预分配缓冲区
        m_buffer.reserve(m_maxLength + 1);
        m_buffer.resize(m_maxLength + 1, '\0');
    }

    // ===================== 文本管理 =====================

    /**
     * @brief 设置文本内容
     * @param text 新的文本内容
     */
    void setText(const std::string& text)
    {
        m_text = text;
        updateBuffer();
    }

    /**
     * @brief 获取当前文本内容
     * @return 当前文本
     */
    [[nodiscard]] std::string getText() const { return m_text; }

    /**
     * @brief 追加文本
     * @param text 要追加的文本
     */
    void appendText(const std::string& text)
    {
        m_text += text;
        updateBuffer();
    }

    /**
     * @brief 清空文本
     */
    void clear()
    {
        m_text.clear();
        updateBuffer();
    }

    // ===================== 属性设置 =====================

    /**
     * @brief 设置是否只读
     * @param readOnly true 为只读，false 为可编辑
     */
    void setReadOnly(bool readOnly) { m_readOnly = readOnly; }

    /**
     * @brief 获取是否只读
     * @return 是否只读
     */
    [[nodiscard]] bool isReadOnly() const { return m_readOnly; }

    /**
     * @brief 设置占位符文本（当文本为空时显示）
     * @param placeholder 占位符文本
     */
    void setPlaceholder(const std::string& placeholder) { m_placeholder = placeholder; }

    /**
     * @brief 获取占位符文本
     * @return 占位符文本
     */
    [[nodiscard]] std::string getPlaceholder() const { return m_placeholder; }

    /**
     * @brief 设置最大文本长度
     * @param maxLength 最大长度
     */
    void setMaxLength(size_t maxLength)
    {
        m_maxLength = maxLength;
        m_buffer.resize(m_maxLength + 1, '\0');
        updateBuffer();
    }

    /**
     * @brief 获取最大文本长度
     * @return 最大长度
     */
    [[nodiscard]] size_t getMaxLength() const { return m_maxLength; }

    /**
     * @brief 设置文本变更回调
     * @param callback 当文本改变时调用的回调函数
     */
    void setOnTextChanged(std::function<void(const std::string&)> callback) { m_onTextChanged = std::move(callback); }

    /**
     * @brief 设置是否自动换行
     * @param wordWrap 是否自动换行
     */
    void setWordWrap(bool wordWrap) { m_wordWrap = wordWrap; }

    /**
     * @brief 获取是否自动换行
     * @return 是否自动换行
     */
    [[nodiscard]] bool isWordWrap() const { return m_wordWrap; }

    // ===================== 尺寸计算 =====================

    ImVec2 calculateSize() override
    {
        if (isFixedSize())
        {
            return Widget::calculateSize();
        }

        // 返回最小尺寸
        return {getMinWidth(), getMinHeight()};
    }

protected:
    void onRender(const ImVec2& position, const ImVec2& size) override
    {
        ImGui::SetCursorPos(position);

        // 设置 ImGui 标志
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_None;

        if (m_readOnly)
        {
            flags |= ImGuiInputTextFlags_ReadOnly;
        }

        if (!m_wordWrap)
        {
            flags |= ImGuiInputTextFlags_NoHorizontalScroll;
        }

        // 允许 Tab 输入
        flags |= ImGuiInputTextFlags_AllowTabInput;

        // 回调标志
        flags |= ImGuiInputTextFlags_CallbackAlways;

        // 渲染多行文本输入框
        std::string label = m_uniqueId;
        if (ImGui::InputTextMultiline(
                label.c_str(), m_buffer.data(), m_buffer.size(), size, flags, inputTextCallback, this))
        {
            // 文本已更改
            m_text = m_buffer.data();

            // 触发回调
            if (m_onTextChanged)
            {
                m_onTextChanged(m_text);
            }
        }

        // 如果文本为空且有占位符，显示占位符
        if (m_text.empty() && !m_placeholder.empty() && !ImGui::IsItemActive())
        {
            ImVec2 textPos = ImGui::GetItemRectMin();
            constexpr float PLACEHOLDER_OFFSET_X = 4.0F; // NOLINT
            constexpr float PLACEHOLDER_OFFSET_Y = 4.0F; // NOLINT
            textPos.x += PLACEHOLDER_OFFSET_X;
            textPos.y += PLACEHOLDER_OFFSET_Y;

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            constexpr float PLACEHOLDER_ALPHA = 0.5F;                                                 // NOLINT
            ImU32 placeholderColor = ImGui::GetColorU32(ImVec4(0.5F, 0.5F, 0.5F, PLACEHOLDER_ALPHA)); // NOLINT
            drawList->AddText(textPos, placeholderColor, m_placeholder.c_str());
        }
    }

private:
    /**
     * @brief 更新内部缓冲区
     */
    void updateBuffer()
    {
        // 确保文本不超过最大长度
        if (m_text.length() > m_maxLength)
        {
            m_text.resize(m_maxLength);
        }

        // 清空缓冲区
        std::fill(m_buffer.begin(), m_buffer.end(), '\0');

        // 复制文本到缓冲区
        std::copy(m_text.begin(), m_text.end(), m_buffer.begin());
    }

    /**
     * @brief ImGui 输入回调函数
     */
    static int inputTextCallback(ImGuiInputTextCallbackData* data)
    {
        auto* textEdit = static_cast<TextEdit*>(data->UserData);
        if (!textEdit)
        {
            return 0;
        }

        // 可以在这里添加额外的文本处理逻辑
        // 例如：限制字符类型、自动完成等

        return 0;
    }

    std::string m_text;                                      // 当前文本内容
    std::string m_placeholder;                               // 占位符文本
    std::string m_uniqueId;                                  // 唯一 ID
    std::vector<char> m_buffer;                              // ImGui 缓冲区
    size_t m_maxLength = 4096;                               // 最大文本长度 NOLINT
    bool m_readOnly = false;                                 // 是否只读
    bool m_wordWrap = true;                                  // 是否自动换行
    std::function<void(const std::string&)> m_onTextChanged; // 文本变更回调
};

} // namespace ui