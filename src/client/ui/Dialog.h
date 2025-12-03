/**
 * ************************************************************************
 *
 * @file Dialog.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-01
 * @version 0.1
 * @brief  对话框组件定义
  模拟 Qt 的对话框组件 QDialog
  支持模态和非模态对话框（模态其实是遮罩）
  支持设置标题和内容
  支持显示和隐藏对话框
  基于ImGui实现对话框渲染
  支持自定义按钮和回调函数
  支持设置对话框大小和位置
  点击遮罩层/外部关闭对话框（可选）
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
#include <string_view>
#include <imgui.h>

namespace ui
{
class Dialog : public Widget
{
public:
    explicit Dialog(std::string_view title = "Dialog") : m_title(title)
    {
        constexpr float DEFAULT_DIALOG_WIDTH = 400.0F;
        constexpr float DEFAULT_DIALOG_HEIGHT = 300.0F;
        setFixedSize(DEFAULT_DIALOG_WIDTH, DEFAULT_DIALOG_HEIGHT);
    }

    ~Dialog() override = default;
    Dialog(const Dialog&) = delete;
    Dialog& operator=(const Dialog&) = delete;
    Dialog(Dialog&&) = delete;
    Dialog& operator=(Dialog&&) = delete;

    // ===================== 对话框控制 =====================
    void show()
    {
        m_open = true;
        setVisible(true);
    }
    void hide()
    {
        m_open = false;
        setVisible(false);
    }
    void close() { hide(); }
    [[nodiscard]] bool isOpen() const { return m_open; }

    // ===================== 模态设置 =====================
    void setModal(bool modal) { m_modal = modal; }
    [[nodiscard]] bool isModal() const { return m_modal; }

    // ===================== 标题和内容 =====================
    void setTitle(const std::string& title) { m_title = title; }
    [[nodiscard]] const std::string& getTitle() const { return m_title; }

    // ===================== 关闭行为 =====================
    void setCloseOnClickOutside(bool enabled) { m_closeOnClickOutside = enabled; }
    [[nodiscard]] bool getCloseOnClickOutside() const { return m_closeOnClickOutside; }

    // ===================== 按钮管理 =====================
    void addButton(const std::string& text, std::function<void()> callback = nullptr)
    {
        m_buttons.push_back({text, std::move(callback)});
    }

    void clearButtons() { m_buttons.clear(); }

    // ===================== 关闭回调 =====================
    void setOnClose(std::function<void()> callback) { m_onClose = std::move(callback); }

protected:
    void onRender(const ImVec2& position, const ImVec2& size) override
    {
        if (!m_open)
        {
            return;
        }

        // 如果是模态对话框，绘制遮罩层
        if (m_modal)
        {
            renderModalOverlay();
        }

        // 设置对话框窗口标志
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
        if (m_modal)
        {
            flags = static_cast<ImGuiWindowFlags>(
                static_cast<unsigned>(flags) |
                static_cast<unsigned>(ImGuiWindowFlags_NoMove)); // 模态对话框不可移动（可选）
        }

        // 设置对话框位置（居中）
        constexpr float CENTER_RATIO = 0.5F;
        ImGuiIO& imguiIO = ImGui::GetIO();
        ImVec2 center(imguiIO.DisplaySize.x * CENTER_RATIO, imguiIO.DisplaySize.y * CENTER_RATIO);
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(CENTER_RATIO, CENTER_RATIO));
        ImGui::SetNextWindowSize(size, ImGuiCond_Appearing);

        // 开始渲染对话框
        if (ImGui::Begin(m_title.c_str(), &m_open, flags))
        {
            // 渲染自定义内容（子Widget）
            renderChildren(ImVec2(0, 0));

            // 渲染底部按钮
            if (!m_buttons.empty())
            {
                ImGui::Separator();
                renderButtons();
            }
        }
        ImGui::End();

        // 检测对话框是否被关闭
        if (!m_open)
        {
            handleClose();
        }

        // 检测点击外部关闭
        if (m_closeOnClickOutside && m_modal && ImGui::IsMouseClicked(0))
        {
            ImVec2 mousePos = ImGui::GetMousePos();
            ImVec2 winPos = ImGui::GetWindowPos();
            ImVec2 winSize = ImGui::GetWindowSize();

            // 检查鼠标是否在对话框外部
            if (mousePos.x < winPos.x || mousePos.x > winPos.x + winSize.x || mousePos.y < winPos.y ||
                mousePos.y > winPos.y + winSize.y)
            {
                close();
            }
        }
    }

private:
    struct DialogButton
    {
        std::string text;
        std::function<void()> callback;
    };

    static void renderModalOverlay()
    {
        // 绘制半透明遮罩层
        constexpr float OVERLAY_ALPHA = 0.5F;
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowBgAlpha(OVERLAY_ALPHA); // 半透明背景

        auto overlayFlags = static_cast<ImGuiWindowFlags>(
            static_cast<unsigned>(ImGuiWindowFlags_NoDecoration) | static_cast<unsigned>(ImGuiWindowFlags_NoMove) |
            static_cast<unsigned>(ImGuiWindowFlags_NoSavedSettings) |
            static_cast<unsigned>(ImGuiWindowFlags_NoFocusOnAppearing) |
            static_cast<unsigned>(ImGuiWindowFlags_NoBringToFrontOnFocus));

        ImGui::Begin("##ModalOverlay", nullptr, overlayFlags);
        ImGui::End();
    }

    void renderButtons()
    {
        // 居中显示按钮
        constexpr float BUTTON_SPACING = 10.0F;
        constexpr float BUTTON_PADDING = 20.0F;
        constexpr float CENTER_RATIO = 0.5F;

        float totalButtonWidth = 0;
        for (const auto& btn : m_buttons)
        {
            ImVec2 textSize = ImGui::CalcTextSize(btn.text.c_str());
            totalButtonWidth += textSize.x + BUTTON_PADDING; // 加上padding
        }
        totalButtonWidth += static_cast<float>(m_buttons.size() - 1) * BUTTON_SPACING;

        float windowWidth = ImGui::GetWindowWidth();
        float startX = (windowWidth - totalButtonWidth) * CENTER_RATIO;
        ImGui::SetCursorPosX(startX);

        // 渲染按钮
        for (size_t i = 0; i < m_buttons.size(); ++i)
        {
            const auto& btn = m_buttons[i];
            if (i > 0)
            {
                ImGui::SameLine();
            }

            if (ImGui::Button(btn.text.c_str()))
            {
                if (btn.callback)
                {
                    btn.callback();
                }
                close();
            }
        }
    }

    void handleClose()
    {
        setVisible(false);
        if (m_onClose)
        {
            m_onClose();
        }
    }

    std::string m_title;
    bool m_modal = false;
    bool m_open = false;
    bool m_closeOnClickOutside = true;
    std::vector<DialogButton> m_buttons;
    std::function<void()> m_onClose;
};
} // namespace ui