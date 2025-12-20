/**
 * ************************************************************************
 *
 * @file RenderSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-11 (Final)
 * @version 0.3
 * @brief UI渲染系统
 *
 * 负责渲染所有UI元素的ECS系统，通过递归遍历和ImGui DrawList实现纯绘制。
 * 特别处理 Window/Dialog 容器，将其映射为 ImGui 窗口实例。
    查找MainWidgetTag标记的主实体作为渲染起点。
    渲染所有具有VisibleTag的实体。
    处理透明度继承和计算。
    使用ImGui DrawList进行低级绘制，避免使用ImGui控件函数。
    递归渲染子实体，支持嵌套布局。
    支持背景色、边框、文本、图像等多种UI元素。
    优化渲染顺序，确保正确的层级关系。
    易于扩展以支持更多UI组件和效果。
 *
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include <imgui.h>
#include <functional>
#include "components/Components.h"
#include "components/Tags.h"
#include <utils.h>
#include <SDL3/SDL.h>

namespace ui::systems
{

class RenderSystem
{
public:
    void update(SDL_Renderer* renderer) noexcept
    {
        // ----------------------------------------------------
        // I. ImGui 顶层画布设置
        // ----------------------------------------------------
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        // 使用一个始终打开的 ImGui 窗口作为根画布 (Root Canvas)
        ImGui::Begin("##ECS_UI_ROOT_CANVAS",
                     nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
                         ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground);

        ImGui::PopStyleVar(2);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // ----------------------------------------------------
        // II. ECS 渲染调度 (遍历顶层元素)
        // ----------------------------------------------------
        auto& registry = utils::Registry::getInstance();

        // 查找所有具有 Position, Size, VisibleTag 且 Parent 为 entt::null 的实体
        auto view = registry.view<const components::Position,
                                  const components::Size,
                                  const components::VisibleTag,
                                  const components::Hierarchy>();

        for (auto entity : view)
        {
            const auto& hierarchy = registry.get<const components::Hierarchy>(entity);

            if (hierarchy.parent == entt::null)
            {
                // 递归渲染。根画布的绝对位置是 (0, 0)
                renderEntityRecursive(registry, entity, draw_list, ImVec2(0, 0), 1.0f);
            }
        }

        ImGui::End();
    }

private:
    /**
     * @brief 递归渲染单个实体及其子实体。
     *
     * @param registry EnTT 注册表
     * @param entity 当前实体
     * @param draw_list ImGui 绘制列表 (取决于当前 ImGui 窗口)
     * @param parentAbsolutePos 父级的**内容区**绝对位置
     * @param parentGlobalAlpha 父级的最终透明度
     */
    void renderEntityRecursive(entt::registry& registry,
                               entt::entity entity,
                               ImDrawList* draw_list,
                               const ImVec2& parentAbsolutePos,
                               float parentGlobalAlpha)
    {
        // 检查可见性 (View 已经过滤了一部分，但递归中仍需检查)
        if (!registry.any_of<components::VisibleTag>(entity)) return;

        // 排除非渲染容器 Tag，如 Spacer
        if (registry.any_of<components::SpacerTag>(entity)) return;

        const auto& pos = registry.get<const components::Position>(entity);
        const auto& size = registry.get<const components::Size>(entity);
        const auto* alphaComp = registry.try_get<components::Alpha>(entity);

        // 计算当前元素的最终透明度
        const float globalAlpha = parentGlobalAlpha * (alphaComp ? alphaComp->value : 1.0f);

        // 计算当前实体的绝对屏幕位置 (相对于父级内容区)
        ImVec2 absolutePos(parentAbsolutePos.x + pos.value.x, parentAbsolutePos.y + pos.value.y);
        ImVec2 absoluteEndPos(absolutePos.x + size.size.x, absolutePos.y + size.size.y);

        // ----------------------------------------------------
        // 1. 容器特殊处理 (Window/Dialog)
        // ----------------------------------------------------
        if (registry.any_of<components::WindowTag>(entity) || registry.any_of<components::DialogTag>(entity))
        {
            // 窗口容器由专门函数处理，它会调用 ImGui::Begin/End 并在内部递归。
            renderWindow(registry, entity, absolutePos, globalAlpha);
            return; // 窗口/对话框处理了自身的绘制和递归，跳过默认处理
        }

        // ----------------------------------------------------
        // 2. 核心渲染 (背景/边框)
        // ----------------------------------------------------
        renderBackground(registry, entity, draw_list, absolutePos, absoluteEndPos, globalAlpha);

        // ----------------------------------------------------
        // 3. 渲染特定内容
        // ----------------------------------------------------
        renderSpecificComponent(registry, entity, draw_list, absolutePos, size.size, globalAlpha);

        // ----------------------------------------------------
        // 4. 递归渲染子元素 (适用于非 Window 的普通容器，如 VBox/HBox)
        // ----------------------------------------------------
        const auto* hierarchy = registry.try_get<components::Hierarchy>(entity);
        if (hierarchy && !hierarchy->children.empty())
        {
            // 对于普通布局容器，子元素绘制在当前 DrawList (draw_list) 中
            for (entt::entity child : hierarchy->children)
            {
                // 递归调用，将当前元素的绝对位置作为子元素的父级绝对位置
                renderEntityRecursive(registry, child, draw_list, absolutePos, globalAlpha);
            }
        }
    }

    // =======================================================
    // 渲染 Helper 函数
    // =======================================================

    /**
     * @brief 渲染 ECS 窗口/对话框实体。
     *
     * 这是一个特殊的渲染函数，因为它负责调用 ImGui::Begin/End，
     * 从而切换 ImGui 的当前绘制上下文和剪裁区域。
     */
    void renderWindow(entt::registry& registry, entt::entity entity, const ImVec2& absolutePos, float globalAlpha)
    {
        const auto& windowComp = registry.get<const components::Window>(entity);
        const auto& size = registry.get<const components::Size>(entity);
        const auto* hierarchy = registry.try_get<components::Hierarchy>(entity);

        // --- 1. 设置 ImGui 窗口状态 ---
        ImGui::SetNextWindowPos(absolutePos);
        ImGui::SetNextWindowSize(size.size);

        ImGuiWindowFlags flags = ImGuiWindowFlags_None;
        if (windowComp.noResize) flags |= ImGuiWindowFlags_NoResize;
        if (windowComp.noMove) flags |= ImGuiWindowFlags_NoMove;
        if (windowComp.noCollapse) flags |= ImGuiWindowFlags_NoCollapse;
        // ImGui 的“模态”语义通常通过 BeginPopupModal 实现；此处使用普通 Begin，不设置不存在的 WindowFlags。

        // 标记：如果 ECS 窗口/对话框没有 Background 组件，则使用 ImGui 默认背景。
        // 如果有 Background 组件，则设置 ImGuiWindowFlags_NoBackground，并在 ECS 中绘制。
        if (registry.any_of<components::Background>(entity))
        {
            flags |= ImGuiWindowFlags_NoBackground;
        }

        // --- 2. 绘制 ImGui 窗口（使用 title 作为 ImGui ID） ---
        if (ImGui::Begin(windowComp.title.c_str(), nullptr, flags))
        {
            ImDrawList* window_draw_list = ImGui::GetWindowDrawList();
            ImVec2 contentStartPos = ImGui::GetCursorScreenPos();

            // 绘制 ECS 提供的自定义背景/边框 (如果设置了 NoBackground)
            ImVec2 absEndPos(absolutePos.x + size.size.x, absolutePos.y + size.size.y);
            renderBackground(registry, entity, window_draw_list, absolutePos, absEndPos, globalAlpha);

            // --- 3. 递归渲染子元素 (子元素相对于窗口内容区定位) ---
            if (hierarchy && !hierarchy->children.empty())
            {
                for (entt::entity child : hierarchy->children)
                {
                    // 递归调用：使用 ImGui 窗口的 DrawList 和 内容区位置
                    renderEntityRecursive(registry, child, window_draw_list, contentStartPos, globalAlpha);
                }
            }
        }
        ImGui::End();
    }

    // 以下 Helper 函数 (renderBackground, renderSpecificComponent) 保持不变，
    // 因为它们是纯粹的 ImDrawList 操作，不需要修改。

    void renderBackground(entt::registry& registry,
                          entt::entity entity,
                          ImDrawList* draw_list,
                          const ImVec2& startPos,
                          const ImVec2& endPos,
                          float globalAlpha)
    {
        // 渲染背景 (Background Component)
        const auto* bg = registry.try_get<components::Background>(entity);
        if (bg && bg->enabled)
        {
            ImVec4 finalColor = bg->color;
            finalColor.w *= globalAlpha;
            ImU32 color = ImGui::GetColorU32(finalColor);

            draw_list->AddRectFilled(startPos, endPos, color, bg->borderRadius, ImDrawFlags_RoundCornersAll);
        }

        // 渲染边框 (Border Component)
        const auto* border = registry.try_get<components::Border>(entity);
        if (border && border->thickness > 0.0f)
        {
            ImVec4 finalColor = border->color;
            finalColor.w *= globalAlpha;
            ImU32 color = ImGui::GetColorU32(finalColor);

            draw_list->AddRect(
                startPos, endPos, color, border->borderRadius, ImDrawFlags_RoundCornersAll, border->thickness);
        }
    }

    void renderSpecificComponent(entt::registry& registry,
                                 entt::entity entity,
                                 ImDrawList* draw_list,
                                 const ImVec2& pos,
                                 const ImVec2& size,
                                 float globalAlpha)
    {
        // ... (Text, Image, TextEdit, Arrow 渲染逻辑保持不变)
        // --- 渲染 Text (用于 Label 和 Button) ---
        if (registry.any_of<components::TextTag>(entity) || registry.any_of<components::ButtonTag>(entity) ||
            registry.any_of<components::LabelTag>(entity))
        {
            const auto* textComp = registry.try_get<components::Text>(entity);
            if (textComp && !textComp->content.empty())
            {
                ImVec4 finalColor = textComp->color;
                finalColor.w *= globalAlpha;
                ImU32 color = ImGui::GetColorU32(finalColor);

                ImVec2 textPos = pos;
                const ImVec2 textSize = ImGui::CalcTextSize(textComp->content.c_str());

                const uint8_t align = static_cast<uint8_t>(textComp->alignment);
                if (align & static_cast<uint8_t>(components::Alignment::HCENTER))
                {
                    textPos.x = pos.x + (size.x - textSize.x) * 0.5f;
                }
                else if (align & static_cast<uint8_t>(components::Alignment::RIGHT))
                {
                    textPos.x = pos.x + (size.x - textSize.x);
                }

                if (align & static_cast<uint8_t>(components::Alignment::VCENTER))
                {
                    textPos.y = pos.y + (size.y - textSize.y) * 0.5f;
                }
                else if (align & static_cast<uint8_t>(components::Alignment::BOTTOM))
                {
                    textPos.y = pos.y + (size.y - textSize.y);
                }

                if (textComp->fontSize > 0.0f)
                {
                    draw_list->AddText(ImGui::GetFont(), textComp->fontSize, textPos, color, textComp->content.c_str());
                }
                else
                {
                    draw_list->AddText(textPos, color, textComp->content.c_str());
                }
            }
        }

        // --- 渲染 Image ---
        if (registry.any_of<components::ImageTag>(entity))
        {
            const auto& imageComp = registry.get<const components::Image>(entity);
            if (imageComp.textureId)
            {
                ImVec2 endPos(pos.x + size.x, pos.y + size.y);
                draw_list->AddImage(imageComp.textureId,
                                    pos,
                                    endPos,
                                    imageComp.uvMin,
                                    imageComp.uvMax,
                                    ImGui::GetColorU32(ImVec4(imageComp.tintColor.x,
                                                              imageComp.tintColor.y,
                                                              imageComp.tintColor.z,
                                                              imageComp.tintColor.w * globalAlpha)));
            }
        }

        // --- 渲染 TextEdit (边框和文本) ---
        if (registry.any_of<components::TextEditTag>(entity))
        {
            const auto& textEditComp = registry.get<const components::TextEdit>(entity);
            ImVec2 endPos(pos.x + size.x, pos.y + size.y);

            // 1. 绘制 TextEdit 背景和边框
            ImU32 bg_color = ImGui::GetColorU32(ImVec4(0.1f, 0.1f, 0.1f, 0.8f * globalAlpha));
            ImU32 border_color = ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, globalAlpha));
            draw_list->AddRectFilled(pos, endPos, bg_color, 4.0f);
            draw_list->AddRect(pos, endPos, border_color, 4.0f, 0, 1.0f);

            // 2. 绘制文本 (buffer 或 placeholder)
            const std::string& content = textEditComp.buffer.empty() ? textEditComp.placeholder : textEditComp.buffer;
            ImVec4 finalColor = textEditComp.textColor;
            finalColor.w *= globalAlpha;
            ImU32 text_color = ImGui::GetColorU32(finalColor);
            ImVec2 textPos(pos.x + 5.0f, pos.y + 5.0f);
            draw_list->AddText(textPos, text_color, content.c_str());
        }

        // --- 渲染 Arrow ---
        if (registry.any_of<components::ArrowTag>(entity))
        {
            const auto& arrowComp = registry.get<const components::Arrow>(entity);
            ImVec4 finalColor = arrowComp.color;
            finalColor.w *= globalAlpha;
            ImU32 color = ImGui::GetColorU32(finalColor);
            draw_list->AddLine(arrowComp.startPoint, arrowComp.endPoint, color, arrowComp.thickness);
        }
    }
};

} // namespace ui::systems