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
    遍历所有可见的UI实体，根据其组件类型调用相应的渲染函数
    使用ImGui进行实际绘制
    支持按钮、标签、文本编辑框、图像等常用UI组件
    处理层级关系，递归渲染子元素
    防止重入，确保渲染过程安全
    可扩展以支持更多UI组件类型
    集成背景绘制和透明度控制
    使用ECS模式管理UI状态和属性
    提供清晰的渲染流程，易于维护和扩展
    确保UI渲染性能和响应速度
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
#include "src/client/utils/utils.h"
namespace ui::systems
{

class UIRenderSystem
{
public:
    /**
     * @brief 渲染所有可见的UI元素
     */
    void update() noexcept
    {
        // 渲染顶层元素（没有父节点的元素）
        auto view =
            utils::Registry::getInstance().view<components::Position, components::Size, components::Visibility>();

        for (auto entity : view)
        {
            const auto& hierarchy = utils::Registry::getInstance().try_get<components::Hierarchy>(entity);

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
    void renderWidget(entt::entity entity) noexcept
    {
        auto& registry = utils::Registry::getInstance();

        const auto* visibility = registry.try_get<components::Visibility>(entity);
        if (!visibility || !visibility->visible || visibility->alpha <= 0.0F)
        {
            return;
        }

        auto* renderState = registry.try_get<components::RenderState>(entity);
        if (!renderState)
        {
            renderState = &registry.emplace<components::RenderState>(entity);
        }

        // 防止重入
        if (renderState->isRendering)
        {
            return;
        }

        renderState->isRendering = true;

        // 获取位置和尺寸
        const auto* position = registry.try_get<components::Position>(entity);
        const auto* size = registry.try_get<components::Size>(entity);

        if (!position || !size)
        {
            renderState->isRendering = false;
            return;
        }

        ImVec2 pos{position->x, position->y};
        ImVec2 sz{size->width, size->height};

        // 渲染背景
        if (const auto* bg = registry.try_get<components::Background>(entity); bg && bg->enabled)
        {
            renderBackground(entity, pos, sz, bg->color, visibility->alpha);
        }

        // 根据组件类型渲染
        if (registry.all_of<components::ButtonTag>(entity))
        {
            renderButton(entity, pos, sz);
        }
        else if (registry.all_of<components::LabelTag>(entity))
        {
            renderLabel(entity, pos, sz);
        }
        else if (registry.all_of<components::TextEditTag>(entity))
        {
            renderTextEdit(entity, pos, sz);
        }
        else if (registry.all_of<components::ImageTag>(entity))
        {
            renderImage(entity, pos, sz);
        }
        else if (registry.all_of<components::ArrowTag>(entity))
        {
            renderArrow(entity, pos, sz);
        }
        else if (registry.all_of<components::LayoutTag>(entity))
        {
            renderLayout(entity, pos, sz);
        }

        // 渲染子元素
        if (const auto* hierarchy = registry.try_get<components::Hierarchy>(entity))
        {
            for (auto child : hierarchy->children)
            {
                if (registry.valid(child))
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
    void renderBackground(const ImVec2& pos, const ImVec2& size, const ImVec4& color, float alpha) noexcept
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec4 finalColor = color;
        finalColor.w *= alpha;
        drawList->AddRectFilled(
            pos, ImVec2(pos.x + size.x, pos.y + size.y), ImGui::ColorConvertFloat4ToU32(finalColor));
    }

    /**
     * @brief 渲染带圆角的背景
     */
    void renderBackground(
        entt::entity entity, const ImVec2& pos, const ImVec2& size, const ImVec4& color, float alpha) noexcept
    {
        auto& registry = utils::Registry::getInstance();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec4 finalColor = color;
        finalColor.w *= alpha;

        const ImVec2 endPos(pos.x + size.x, pos.y + size.y);
        const auto* rounded = registry.try_get<components::RoundedCorners>(entity);

        if (rounded && rounded->radius > 0.0F)
        {
            drawList->AddRectFilled(
                pos, endPos, ImGui::ColorConvertFloat4ToU32(finalColor), rounded->radius, rounded->getDrawFlags());
        }
        else
        {
            drawList->AddRectFilled(pos, endPos, ImGui::ColorConvertFloat4ToU32(finalColor));
        }
    }

    /**
     * @brief 渲染按钮
     */
    void renderButton(entt::entity entity, const ImVec2& pos, const ImVec2& size) noexcept
    {
        auto* clickable = utils::Registry::getInstance().try_get<components::Clickable>(entity);
        if (!clickable)
        {
            return;
        }

        ImGui::SetCursorPos(pos);

        // 禁用状态样式
        if (!clickable->enabled)
        {
            constexpr float DISABLED_ALPHA = 0.5F;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * DISABLED_ALPHA);
        }

        std::string label = clickable->text + clickable->uniqueId;
        bool clicked = ImGui::Button(label.c_str(), size);

        // 处理点击
        if (clicked && clickable->enabled)
        {
            utils::Dispatcher::getInstance().enqueue<events::ButtonClicked>(entity);
        }

        // 显示提示文本
        if (!clickable->tooltip.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        {
            ImGui::SetTooltip("%s", clickable->tooltip.c_str());
        }

        // 恢复样式
        if (!clickable->enabled)
        {
            ImGui::PopStyleVar();
        }
    }

    /**
     * @brief 渲染文本标签
     */
    void renderLabel(entt::entity entity, const ImVec2& pos, const ImVec2& size) noexcept
    {
        auto* showText = utils::Registry::getInstance().try_get<components::ShowText>(entity);
        if (!showText)
        {
            return;
        }

        ImGui::SetCursorPos(pos);
        ImGui::PushStyleColor(ImGuiCol_Text, showText->textColor);

        if (showText->wordWrap)
        {
            ImGui::PushTextWrapPos(showText->wrapWidth > 0 ? pos.x + showText->wrapWidth : pos.x + size.x);
            ImGui::TextWrapped("%s", showText->text.c_str());
            ImGui::PopTextWrapPos();
        }
        else
        {
            ImGui::Text("%s", showText->text.c_str());
        }

        ImGui::PopStyleColor();
    }

    /**
     * @brief 渲染文本编辑框
     */
    void renderTextEdit(entt::entity entity, const ImVec2& pos, const ImVec2& size) noexcept
    {
        auto& textEdit = utils::Registry::getInstance().get<components::TextEdit>(entity);

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
        auto& image = utils::Registry::getInstance().get<components::Image>(entity);

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
        auto& arrow = utils::Registry::getInstance().get<components::Arrow>(entity);

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
        auto& layout = utils::Registry::getInstance().get<components::Layout>(entity);

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
            if (item.widget != entt::null && utils::Registry::getInstance().valid(item.widget))
            {
                if (item.stretchFactor == 0)
                {
                    const auto* itemSize = utils::Registry::getInstance().try_get<components::Size>(item.widget);
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

        size_t widgetCount =
            std::count_if(layout.items.begin(),
                          layout.items.end(),
                          [this](const components::LayoutItem& item)
                          { return item.widget != entt::null && utils::Registry::getInstance().valid(item.widget); });
        float totalSpacing = widgetCount > 1 ? layout.spacing * static_cast<float>(widgetCount - 1) : 0.0F;
        float stretchWidth = totalStretch > 0
                                 ? (availableWidth - totalFixedWidth - totalSpacing) / static_cast<float>(totalStretch)
                                 : 0.0F;

        float currentX = contentPos.x;

        for (const auto& item : layout.items)
        {
            if (item.widget == entt::null || !utils::Registry::getInstance().valid(item.widget))
            {
                currentX += stretchWidth * static_cast<float>(item.stretchFactor);
                continue;
            }

            auto* itemPos = utils::Registry::getInstance().try_get<components::Position>(item.widget);
            auto* itemSize = utils::Registry::getInstance().try_get<components::Size>(item.widget);

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
            if (item.widget != entt::null && utils::Registry::getInstance().valid(item.widget))
            {
                if (item.stretchFactor == 0)
                {
                    const auto* itemSize = utils::Registry::getInstance().try_get<components::Size>(item.widget);
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

        size_t widgetCount =
            std::count_if(layout.items.begin(),
                          layout.items.end(),
                          [this](const components::LayoutItem& item)
                          { return item.widget != entt::null && utils::Registry::getInstance().valid(item.widget); });
        float totalSpacing = widgetCount > 1 ? layout.spacing * static_cast<float>(widgetCount - 1) : 0.0F;
        float stretchHeight =
            totalStretch > 0 ? (availableHeight - totalFixedHeight - totalSpacing) / static_cast<float>(totalStretch)
                             : 0.0F;

        float currentY = contentPos.y;

        for (const auto& item : layout.items)
        {
            if (item.widget == entt::null || !utils::Registry::getInstance().valid(item.widget))
            {
                currentY += stretchHeight * static_cast<float>(item.stretchFactor);
                continue;
            }

            auto* itemPos = utils::Registry::getInstance().try_get<components::Position>(item.widget);
            auto* itemSize = utils::Registry::getInstance().try_get<components::Size>(item.widget);

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
};

} // namespace ui::systems
