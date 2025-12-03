/**
 * ************************************************************************
 *
 * @file Widget.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-01
 * @version 0.1
 * @brief 基础组件类定义
  模拟 Qt 的组件基类 QWidget
  所有UI组件均继承自该类
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include <cfloat>
#include <cassert>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
namespace ui
{
class Widget
{
public:
    using Ptr = std::shared_ptr<Widget>;

    Widget() = default;
    virtual ~Widget() = default;
    Widget(const Widget&) = default;
    Widget& operator=(const Widget&) = default;
    Widget(Widget&&) = default;
    Widget& operator=(Widget&&) = default;

    // ===================== 尺寸和位置 =====================
    void setFixedSize(float width, float height)
    {
        m_fixedWidth = width;
        m_fixedHeight = height;
        m_useFixedSize = true;
    }

    void unsetFixedSize() { m_useFixedSize = false; }

    void setPosition(float posX, float posY)
    {
        m_positionX = posX;
        m_positionY = posY;
        m_useCustomPosition = true;
    }

    void setMinimumSize(float width, float height)
    {
        m_minWidth = width;
        m_minHeight = height;
    }

    void setMaximumSize(float width, float height)
    {
        m_maxWidth = width;
        m_maxHeight = height;
    }

    // ===================== 可见性 =====================
    void setVisible(bool visible) { m_visible = visible; }
    [[nodiscard]] bool isVisible() const { return m_visible; }

    // ===================== Checkable =====================
    void setCheckable(bool checkable) { m_checkable = checkable; }
    [[nodiscard]] bool isCheckable() const { return m_checkable; }
    void setChecked(bool checked) { m_checked = checked; }
    [[nodiscard]] bool isChecked() const { return m_checked; }

    // ===================== 透明度 =====================
    void setAlpha(float alpha) { m_alpha = std::clamp(alpha, 0.0F, 1.0F); }
    [[nodiscard]] float getAlpha() const { return m_alpha; }

    // ===================== 背景 =====================
    void setBackgroundColor(const ImVec4& color) { m_backgroundColor = color; }
    void setBackgroundEnabled(bool enabled) { m_drawBackground = enabled; }
    [[nodiscard]] const ImVec4& getBackgroundColor() const { return m_backgroundColor; }
    [[nodiscard]] bool isBackgroundEnabled() const { return m_drawBackground; }

    // ===================== 子 Widget 管理 =====================
    void addChild(Ptr child)
    {
        // 防止创建父子环（不允许把自己或自己的祖先加入为子节点）
        if (!child)
        {
            return;
        }
        if (child.get() == this)
        {
            assert(false && "Cannot add widget as its own child");
            return;
        }
        if (child->hasDescendant(this))
        {
            assert(false && "Cannot add ancestor as a child (would form a cycle)");
            return;
        }
        m_children.push_back(std::move(child));
    }
    [[nodiscard]] const std::vector<Ptr>& getChildren() const { return m_children; }

    // ===================== 渲染 =====================
    // 注意：此函数递归调用子 widget 的 render，这是树形遍历的预期行为
    // 通过 addChild 的环检测和 m_isRendering 重入保护避免无限递归
    // NOLINTNEXTLINE
    void render(const ImVec2& position, const ImVec2& size)
    {
        if (!m_visible || m_alpha <= 0.0F)
        {
            return;
        }

        // 防止同一 widget 在渲染链中被重复调用（例如错误的父子引用形成环）
        if (m_isRendering)
        {
            assert(false && "Re-entrant render detected: possible widget cycle");
            return;
        }

        // RAII guard 确保退出时重置标志
        struct RenderGuard
        {
            bool& flagRef;
            explicit RenderGuard(bool& flag) : flagRef(flag) { flagRef = true; }
            ~RenderGuard() { flagRef = false; }
            RenderGuard(const RenderGuard&) = delete;
            RenderGuard& operator=(const RenderGuard&) = delete;
            RenderGuard(RenderGuard&&) = delete;
            RenderGuard& operator=(RenderGuard&&) = delete;
        } renderGuard(m_isRendering);

        ImVec2 finalSize = size;
        if (m_useFixedSize)
        {
            finalSize.x = m_fixedWidth;
            finalSize.y = m_fixedHeight;
        }
        finalSize.x = std::clamp(finalSize.x, m_minWidth, m_maxWidth);
        finalSize.y = std::clamp(finalSize.y, m_minHeight, m_maxHeight);

        constexpr float ALPHA_THRESHOLD = 0.999F;
        bool pushAlpha = m_alpha < ALPHA_THRESHOLD;
        if (pushAlpha)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * m_alpha);
        }

        ImVec2 effectivePos = position;
        if (m_useCustomPosition)
        {
            effectivePos.x = m_positionX;
            effectivePos.y = m_positionY;
        }

        ImGui::SetCursorPos(effectivePos); // 设置光标位置为有效位置

        // 设置裁剪区域，确保子组件不会超出父组件边界
        ImVec2 clipMin = ImGui::GetCursorScreenPos();
        ImVec2 clipMax = ImVec2(clipMin.x + finalSize.x, clipMin.y + finalSize.y);
        ImGui::PushClipRect(clipMin, clipMax, true);

        // 绘制背景（如果启用）
        if (m_drawBackground)
        {
            if (ImDrawList* drawList = ImGui::GetWindowDrawList())
            {
                ImVec2 topLeft = ImGui::GetCursorScreenPos();
                ImVec2 bottomRight = ImVec2(topLeft.x + finalSize.x, topLeft.y + finalSize.y);
                ImVec4 bgColor = m_backgroundColor;
                bgColor.w *= m_alpha; // 应用透明度
                drawList->AddRectFilled(topLeft, bottomRight, ImGui::GetColorU32(bgColor));
            }
        }

        onRender(effectivePos, finalSize);

        // 恢复裁剪区域
        ImGui::PopClipRect();

        if (pushAlpha)
        {
            ImGui::PopStyleVar(); // 恢复透明度
        }
    }

    // ===================== 尺寸计算 =====================
    // 注意：此函数递归调用子 widget 的 calculateSize，这是树形遍历的预期行为
    // NOLINTNEXTLINE
    virtual ImVec2 calculateSize()
    {
        if (m_useFixedSize)
        {
            return {m_fixedWidth, m_fixedHeight};
        }

        // 如果有子 Widget，则计算总高度和最大宽度
        float width = m_minWidth;
        float height = 0.0F;
        for (auto& child : m_children)
        {
            ImVec2 childSize = child->calculateSize();
            width = std::max(width, childSize.x);
            height += childSize.y;
        }
        width = std::clamp(width, m_minWidth, m_maxWidth);
        height = std::clamp(height, m_minHeight, m_maxHeight);
        return {width, height};
    }

    [[nodiscard]] float getMinWidth() const { return m_minWidth; }
    [[nodiscard]] float getMinHeight() const { return m_minHeight; }
    [[nodiscard]] float getMaxWidth() const { return m_maxWidth; }
    [[nodiscard]] float getMaxHeight() const { return m_maxHeight; }
    [[nodiscard]] bool isFixedSize() const { return m_useFixedSize; }

protected:
    virtual void onRender(const ImVec2& position, const ImVec2& size) = 0;

    // 受保护的方法：如果子类需要渲染 m_children，可以调用此方法
    void renderChildren(const ImVec2& basePosition)
    {
        ImVec2 childPos = basePosition;
        for (auto& child : m_children)
        {
            if (!child->isVisible())
            {
                continue;
            }

            ImVec2 childSize = child->calculateSize();
            child->render(childPos, childSize);
            childPos.y += childSize.y;
        }
    }

private:
    std::vector<Ptr> m_children;

    // 递归检查是否包含指定的后代 widget（用于环检测）
    // 这是深度优先搜索，递归是预期行为
    // NOLINTNEXTLINE
    bool hasDescendant(const Widget* target) const
    {
        return std::ranges::any_of(m_children,
                                   [target](const auto& child)
                                   { return child && (child.get() == target || child->hasDescendant(target)); });
    }

    bool m_useFixedSize = false;
    float m_fixedWidth = 0, m_fixedHeight = 0;
    float m_minWidth = 0, m_minHeight = 0;
    float m_maxWidth = FLT_MAX, m_maxHeight = FLT_MAX;
    bool m_visible = true;
    bool m_checkable = false;
    bool m_checked = false;
    float m_alpha = 1.0F;

    float m_positionX = 0.0F;
    float m_positionY = 0.0F;
    bool m_useCustomPosition = false;
    bool m_isRendering = false;

    bool m_drawBackground = false;
    ImVec4 m_backgroundColor{1.0F, 1.0F, 1.0F, 1.0F}; // 默认白色
};
} // namespace ui