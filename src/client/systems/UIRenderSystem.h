/**
 * ************************************************************************
 *
 * @file UIRenderSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief UI渲染系统
 *
 * 负责渲染所有UI元素的ECS系统
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include <imgui.h>
#include "src/client/components/UIComponents.h"
#include "src/client/events/UIEvents.h"
#include "src/client/utils/Dispatcher.h"

namespace ui::systems
{

class UIRenderSystem
{
public:
    explicit UIRenderSystem(entt::registry& registry) : m_registry(registry) {}

    /**
     * @brief 渲染所有可见的UI元素
     */
    void update()
    {
        // 渲染顶层元素（没有父节点的元素）
        auto view = m_registry.view<components::Position, components::Size, components::Visibility>();

        for (auto entity : view)
        {
            const auto& hierarchy = m_registry.try_get<components::Hierarchy>(entity);

            // 只渲染顶层元素（没有父节点或父节点为null）
            if (!hierarchy || hierarchy->parent == entt::null)
            {
                renderWidget(entity);
            }
        }
    }

private:
    /**
     * @brief 递归渲染UI元素及其子元素
     */
    void renderWidget(entt::entity entity)
    {
        auto* visibility = m_registry.try_get<components::Visibility>(entity);
        if (!visibility || !visibility->visible || visibility->alpha <= 0.0F)
        {
            return;
        }

        auto* renderState = m_registry.try_get<components::RenderState>(entity);
        if (!renderState)
        {
            m_registry.emplace<components::RenderState>(entity);
            renderState = m_registry.try_get<components::RenderState>(entity);
        }

        // 防止重入
        if (renderState->isRendering)
        {
            return;
        }

        renderState->isRendering = true;

        // 获取位置和尺寸
        const auto& position = m_registry.get<components::Position>(entity);
        const auto& size = m_registry.get<components::Size>(entity);

        ImVec2 pos{position.x, position.y};
        ImVec2 sz{size.width, size.height};

        // 渲染背景
        if (auto* bg = m_registry.try_get<components::Background>(entity); bg && bg->enabled)
        {
            renderBackground(pos, sz, bg->color, visibility->alpha);
        }

        // 根据组件类型渲染
        if (m_registry.all_of<components::Button>(entity))
        {
            renderButton(entity, pos, sz);
        }
        else if (m_registry.all_of<components::Label>(entity))
        {
            renderLabel(entity, pos, sz);
        }
        else if (m_registry.all_of<components::TextEdit>(entity))
        {
            renderTextEdit(entity, pos, sz);
        }
        else if (m_registry.all_of<components::Image>(entity))
        {
            renderImage(entity, pos, sz);
        }
        else if (m_registry.all_of<components::Arrow>(entity))
        {
            renderArrow(entity, pos, sz);
        }
        else if (m_registry.all_of<components::Layout>(entity))
        {
            renderLayout(entity, pos, sz);
        }

        // 渲染子元素
        if (auto* hierarchy = m_registry.try_get<components::Hierarchy>(entity))
        {
            for (auto child : hierarchy->children)
            {
                if (m_registry.valid(child))
                {
                    renderWidget(child);
                }
            }
        }

        renderState->isRendering = false;
    }

    /**
     * @brief 渲染背景
     */
    void renderBackground(const ImVec2& pos, const ImVec2& size, const ImVec4& color, float alpha)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec4 finalColor = color;
        finalColor.w *= alpha;
        drawList->AddRectFilled(
            pos, ImVec2(pos.x + size.x, pos.y + size.y), ImGui::ColorConvertFloat4ToU32(finalColor));
    }

    /**
     * @brief 渲染按钮
     */
    void renderButton(entt::entity entity, const ImVec2& pos, const ImVec2& size)
    {
        auto& button = m_registry.get<components::Button>(entity);

        ImGui::SetCursorPos(pos);

        // 应用自定义颜色
        int colorsPushed = 0;
        if (button.useCustomColor)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, button.buttonColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button.hoverColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, button.activeColor);
            colorsPushed = 3;
        }

        // 禁用状态样式
        if (!button.enabled)
        {
            constexpr float DISABLED_ALPHA = 0.5F;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * DISABLED_ALPHA);
        }

        std::string label = button.text + button.uniqueId;
        bool clicked = ImGui::Button(label.c_str(), size);

        // 处理点击
        if (clicked && button.enabled && button.onClick)
        {
            button.onClick();
            utils::Dispatcher::getInstance().enqueue<events::ButtonClicked>(entity);
        }

        // 显示提示文本
        if (!button.tooltip.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        {
            ImGui::SetTooltip("%s", button.tooltip.c_str());
        }

        // 恢复样式
        if (!button.enabled)
        {
            ImGui::PopStyleVar();
        }

        if (colorsPushed > 0)
        {
            ImGui::PopStyleColor(colorsPushed);
        }
    }

    /**
     * @brief 渲染文本标签
     */
    void renderLabel(entt::entity entity, const ImVec2& pos, const ImVec2& size)
    {
        auto& label = m_registry.get<components::Label>(entity);

        ImGui::SetCursorPos(pos);
        ImGui::PushStyleColor(ImGuiCol_Text, label.textColor);

        if (label.wordWrap)
        {
            ImGui::PushTextWrapPos(label.wrapWidth > 0 ? pos.x + label.wrapWidth : pos.x + size.x);
            ImGui::TextWrapped("%s", label.text.c_str());
            ImGui::PopTextWrapPos();
        }
        else
        {
            ImGui::Text("%s", label.text.c_str());
        }

        ImGui::PopStyleColor();
    }

    /**
     * @brief 渲染文本编辑框
     */
    void renderTextEdit(entt::entity entity, const ImVec2& pos, const ImVec2& size)
    {
        auto& textEdit = m_registry.get<components::TextEdit>(entity);

        ImGui::SetCursorPos(pos);

        std::string buffer = textEdit.text;
        buffer.resize(textEdit.maxLength);

        ImGuiInputTextFlags flags = 0;
        if (textEdit.readOnly)
        {
            flags |= ImGuiInputTextFlags_ReadOnly;
        }
        if (textEdit.password)
        {
            flags |= ImGuiInputTextFlags_Password;
        }

        bool changed = false;
        if (textEdit.multiline)
        {
            changed = ImGui::InputTextMultiline(textEdit.uniqueId.c_str(), buffer.data(), buffer.size(), size, flags);
        }
        else
        {
            changed = ImGui::InputText(textEdit.uniqueId.c_str(), buffer.data(), buffer.size(), flags);
        }

        if (changed)
        {
            textEdit.text = buffer.c_str();
            if (textEdit.onTextChanged)
            {
                textEdit.onTextChanged(textEdit.text);
            }
            utils::Dispatcher::getInstance().enqueue<events::TextChanged>(entity, textEdit.text);
        }
    }

    /**
     * @brief 渲染图像
     */
    void renderImage(entt::entity entity, const ImVec2& pos, const ImVec2& size)
    {
        auto& image = m_registry.get<components::Image>(entity);

        ImGui::SetCursorPos(pos);

        if (image.textureId)
        {
            ImGui::Image(image.textureId, size, image.uvMin, image.uvMax, image.tintColor, image.borderColor);
        }
    }

    /**
     * @brief 渲染箭头
     */
    void renderArrow(entt::entity entity, const ImVec2& pos, const ImVec2& /* size */)
    {
        auto& arrow = m_registry.get<components::Arrow>(entity);

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        ImVec2 start = ImVec2(pos.x + arrow.startPoint.x, pos.y + arrow.startPoint.y);
        ImVec2 end = ImVec2(pos.x + arrow.endPoint.x, pos.y + arrow.endPoint.y);

        // 绘制线条
        drawList->AddLine(start, end, ImGui::ColorConvertFloat4ToU32(arrow.color), arrow.thickness);

        // 计算箭头方向
        ImVec2 direction = ImVec2(end.x - start.x, end.y - start.y);
        float length = sqrtf(direction.x * direction.x + direction.y * direction.y);
        if (length > 0)
        {
            direction.x /= length;
            direction.y /= length;

            // 绘制箭头头部
            constexpr float ARROW_ANGLE = 0.5F;
            ImVec2 arrowPoint1 =
                ImVec2(end.x - arrow.arrowSize * (direction.x * cosf(ARROW_ANGLE) + direction.y * sinf(ARROW_ANGLE)),
                       end.y - arrow.arrowSize * (direction.y * cosf(ARROW_ANGLE) - direction.x * sinf(ARROW_ANGLE)));
            ImVec2 arrowPoint2 =
                ImVec2(end.x - arrow.arrowSize * (direction.x * cosf(ARROW_ANGLE) - direction.y * sinf(ARROW_ANGLE)),
                       end.y - arrow.arrowSize * (direction.y * cosf(ARROW_ANGLE) + direction.x * sinf(ARROW_ANGLE)));

            drawList->AddTriangleFilled(end, arrowPoint1, arrowPoint2, ImGui::ColorConvertFloat4ToU32(arrow.color));
        }
    }

    /**
     * @brief 渲染布局
     */
    void renderLayout(entt::entity entity, const ImVec2& pos, const ImVec2& size)
    {
        auto& layout = m_registry.get<components::Layout>(entity);

        if (layout.items.empty())
        {
            return;
        }

        if (layout.direction == components::LayoutDirection::HORIZONTAL)
        {
            renderHorizontalLayout(entity, pos, size, layout);
        }
        else
        {
            renderVerticalLayout(entity, pos, size, layout);
        }
    }

    /**
     * @brief 渲染水平布局
     */
    void renderHorizontalLayout(entt::entity /* entity */,
                                const ImVec2& pos,
                                const ImVec2& size,
                                components::Layout& layout)
    {
        ImVec2 contentPos(pos.x + layout.margins.x, pos.y + layout.margins.y);
        float availableWidth = size.x - layout.margins.x - layout.margins.z;
        float availableHeight = size.y - layout.margins.y - layout.margins.w;

        // 计算固定宽度和弹性空间
        float totalFixedWidth = 0;
        int totalStretch = 0;

        for (const auto& item : layout.items)
        {
            if (item.widget != entt::null && m_registry.valid(item.widget))
            {
                if (item.stretchFactor == 0)
                {
                    const auto* itemSize = m_registry.try_get<components::Size>(item.widget);
                    if (itemSize)
                    {
                        totalFixedWidth += itemSize->width;
                    }
                }
                else
                {
                    totalStretch += item.stretchFactor;
                }
            }
            else
            {
                totalStretch += item.stretchFactor;
            }
        }

        size_t widgetCount = std::count_if(layout.items.begin(),
                                           layout.items.end(),
                                           [this](const components::LayoutItem& item)
                                           { return item.widget != entt::null && m_registry.valid(item.widget); });
        float totalSpacing = widgetCount > 1 ? layout.spacing * static_cast<float>(widgetCount - 1) : 0.0F;
        float stretchWidth = totalStretch > 0
                                 ? (availableWidth - totalFixedWidth - totalSpacing) / static_cast<float>(totalStretch)
                                 : 0.0F;

        float currentX = contentPos.x;

        for (const auto& item : layout.items)
        {
            if (item.widget == entt::null || !m_registry.valid(item.widget))
            {
                currentX += stretchWidth * static_cast<float>(item.stretchFactor);
                continue;
            }

            auto* itemPos = m_registry.try_get<components::Position>(item.widget);
            auto* itemSize = m_registry.try_get<components::Size>(item.widget);

            if (!itemPos || !itemSize)
            {
                continue;
            }

            float itemWidth =
                item.stretchFactor > 0 ? stretchWidth * static_cast<float>(item.stretchFactor) : itemSize->width;
            float itemHeight = availableHeight;

            itemPos->x = currentX;
            itemPos->y = contentPos.y;
            itemSize->width = itemWidth;
            itemSize->height = itemHeight;

            renderWidget(item.widget);

            currentX += itemWidth + layout.spacing;
        }
    }

    /**
     * @brief 渲染垂直布局
     */
    void renderVerticalLayout(entt::entity /* entity */,
                              const ImVec2& pos,
                              const ImVec2& size,
                              components::Layout& layout)
    {
        ImVec2 contentPos(pos.x + layout.margins.x, pos.y + layout.margins.y);
        float availableWidth = size.x - layout.margins.x - layout.margins.z;
        float availableHeight = size.y - layout.margins.y - layout.margins.w;

        // 计算固定高度和弹性空间
        float totalFixedHeight = 0;
        int totalStretch = 0;

        for (const auto& item : layout.items)
        {
            if (item.widget != entt::null && m_registry.valid(item.widget))
            {
                if (item.stretchFactor == 0)
                {
                    const auto* itemSize = m_registry.try_get<components::Size>(item.widget);
                    if (itemSize)
                    {
                        totalFixedHeight += itemSize->height;
                    }
                }
                else
                {
                    totalStretch += item.stretchFactor;
                }
            }
            else
            {
                totalStretch += item.stretchFactor;
            }
        }

        size_t widgetCount = std::count_if(layout.items.begin(),
                                           layout.items.end(),
                                           [this](const components::LayoutItem& item)
                                           { return item.widget != entt::null && m_registry.valid(item.widget); });
        float totalSpacing = widgetCount > 1 ? layout.spacing * static_cast<float>(widgetCount - 1) : 0.0F;
        float stretchHeight =
            totalStretch > 0 ? (availableHeight - totalFixedHeight - totalSpacing) / static_cast<float>(totalStretch)
                             : 0.0F;

        float currentY = contentPos.y;

        for (const auto& item : layout.items)
        {
            if (item.widget == entt::null || !m_registry.valid(item.widget))
            {
                currentY += stretchHeight * static_cast<float>(item.stretchFactor);
                continue;
            }

            auto* itemPos = m_registry.try_get<components::Position>(item.widget);
            auto* itemSize = m_registry.try_get<components::Size>(item.widget);

            if (!itemPos || !itemSize)
            {
                continue;
            }

            float itemWidth = availableWidth;
            float itemHeight =
                item.stretchFactor > 0 ? stretchHeight * static_cast<float>(item.stretchFactor) : itemSize->height;

            itemPos->x = contentPos.x;
            itemPos->y = currentY;
            itemSize->width = itemWidth;
            itemSize->height = itemHeight;

            renderWidget(item.widget);

            currentY += itemHeight + layout.spacing;
        }
    }

private:
    entt::registry& m_registry;
};

} // namespace ui::systems
