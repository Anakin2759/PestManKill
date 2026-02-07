/**
 * ************************************************************************
 *
 * @file ShapeRenderer.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-30
 * @version 0.1
 * @brief 形状渲染器 - 处理矩形、圆角、阴影等基础形状渲染
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "../interface/IRenderer.hpp"
#include "../singleton/Registry.hpp"
#include "../common/Components.hpp"
#include "../common/Tags.hpp"
#include "../managers/BatchManager.hpp"
#include "../managers/DeviceManager.hpp"
#include <SDL3/SDL_gpu.h>

namespace ui::renderers
{

/**
 * @brief 形状渲染器
 *
 * 负责渲染：
 * - 背景矩形
 * - 圆角矩形
 * - 阴影效果
 * - 边框
 */
class ShapeRenderer : public core::IRenderer
{
public:
    ShapeRenderer() = default;

    [[nodiscard]] bool canHandle(entt::entity entity) const override
    {
        // 任何有背景或边框的实体都需要形状渲染
        return Registry::AnyOf<components::Background, components::Border>(entity);
    }

    void collect(entt::entity entity, core::RenderContext& context) override
    {
        if (!context.batchManager || !context.deviceManager || !context.whiteTexture)
        {
            return;
        }

        // 渲染背景
        renderBackground(entity, context);

        // 渲染边框
        renderBorder(entity, context);
    }

    int getPriority() const override
    {
        return 0; // 背景应该最先渲染
    }

private:
    void renderBackground(entt::entity entity, core::RenderContext& context)
    {
        const auto* bg = Registry::TryGet<components::Background>(entity);
        if (!bg || bg->enabled != policies::Feature::Enabled)
        {
            return;
        }

        // 准备推送常量
        render::UiPushConstants pushConstants{};
        pushConstants.screen_size[0] = context.screenWidth;
        pushConstants.screen_size[1] = context.screenHeight;
        pushConstants.rect_size[0] = context.size.x();
        pushConstants.rect_size[1] = context.size.y();
        pushConstants.radius[0] = bg->borderRadius.x();
        pushConstants.radius[1] = bg->borderRadius.y();
        pushConstants.radius[2] = bg->borderRadius.z();
        pushConstants.radius[3] = bg->borderRadius.w();
        pushConstants.opacity = context.alpha;

        // 处理阴影
        const auto* shadow = Registry::TryGet<components::Shadow>(entity);
        if (shadow && shadow->enabled == policies::Feature::Enabled)
        {
            pushConstants.shadow_soft = shadow->softness;
            pushConstants.shadow_offset_x = shadow->offset.x();
            pushConstants.shadow_offset_y = shadow->offset.y();
        }
        else
        {
            pushConstants.shadow_soft = 0.0f;
            pushConstants.shadow_offset_x = 0.0f;
            pushConstants.shadow_offset_y = 0.0f;
        }

        // 开始批次
        context.batchManager->beginBatch(context.whiteTexture, context.currentScissor, pushConstants);

        // 添加矩形
        Eigen::Vector4f color(bg->color.red, bg->color.green, bg->color.blue, bg->color.alpha);
        context.batchManager->addRect(context.position, context.size, color);
    }

    void renderBorder(entt::entity entity, core::RenderContext& context)
    {
        const auto* border = Registry::TryGet<components::Border>(entity);
        bool focused = Registry::AnyOf<components::FocusedTag>(entity);

        if (!focused && (!border || border->thickness <= 0.0f))
        {
            return;
        }

        Eigen::Vector4f color(0.0f, 0.0f, 0.0f, 1.0f);
        Eigen::Vector4f radius(0.0f, 0.0f, 0.0f, 0.0f);
        float thickness = 0.0f;

        if (border)
        {
            color = Eigen::Vector4f(border->color.red, border->color.green, border->color.blue, border->color.alpha);
            radius = Eigen::Vector4f(
                border->borderRadius.x(), border->borderRadius.y(), border->borderRadius.z(), border->borderRadius.w());
            thickness = border->thickness;
        }

        if (focused)
        {
            color = Eigen::Vector4f(0.2f, 0.6f, 1.0f, 1.0f); // 蓝色焦点环
            if (thickness < 2.0f) thickness = 2.0f;
        }

        if (thickness > 0.0f)
        {
            renderBorderLines(context, color, thickness);
        }
    }

    void renderBorderLines(core::RenderContext& context, const Eigen::Vector4f& color, float thickness)
    {
        render::UiPushConstants pushConstants{};
        pushConstants.screen_size[0] = context.screenWidth;
        pushConstants.screen_size[1] = context.screenHeight;
        pushConstants.rect_size[0] = context.size.x();
        pushConstants.rect_size[1] = context.size.y();
        pushConstants.opacity = context.alpha;

        context.batchManager->beginBatch(context.whiteTexture, context.currentScissor, pushConstants);

        const Eigen::Vector2f& pos = context.position;
        const Eigen::Vector2f& size = context.size;
        const float halfThickness = thickness * 0.5f;

        // 顶边
        context.batchManager->addRect({pos.x(), pos.y() - halfThickness}, {size.x(), thickness}, color);

        // 右边
        context.batchManager->addRect({pos.x() + size.x() - halfThickness, pos.y()}, {thickness, size.y()}, color);

        // 底边
        context.batchManager->addRect({pos.x(), pos.y() + size.y() - halfThickness}, {size.x(), thickness}, color);

        // 左边
        context.batchManager->addRect({pos.x() - halfThickness, pos.y()}, {thickness, size.y()}, color);
    }
};

} // namespace ui::renderers
