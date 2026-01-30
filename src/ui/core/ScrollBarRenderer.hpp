/**
 * ************************************************************************
 *
 * @file ScrollBarRenderer.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-30
 * @version 0.1
 * @brief 滚动条渲染器 - 处理滚动条的渲染
 *
 * ************************************************************************
 * @copyright Copyright (c) 2026 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "../core/IRenderer.hpp"
#include "../singleton/Registry.hpp"
#include "../common/Components.hpp"
#include "../common/Tags.hpp"
#include "../managers/BatchManager.hpp"
#include <SDL3/SDL_gpu.h>

namespace ui::renderers
{

/**
 * @brief 滚动条渲染器
 *
 * 负责渲染：
 * - 垂直滚动条
 * - 水平滚动条
 */
class ScrollBarRenderer : public core::IRenderer
{
public:
    ScrollBarRenderer(SDL_GPUTexture* whiteTexture) : m_whiteTexture(whiteTexture) {}

    bool canHandle(entt::entity entity) const override { return Registry::AnyOf<components::ScrollArea>(entity); }

    void collect(entt::entity entity, core::RenderContext& context) override
    {
        if (!context.batchManager || !context.deviceManager)
        {
            return;
        }

        const auto* scrollArea = Registry::TryGet<components::ScrollArea>(entity);
        if (!scrollArea || scrollArea->showScrollbars == policies::ScrollBarVisibility::AlwaysOff) return;

        // 渲染滚动条 (在裁剪之前)
        drawScrollBars(entity, context.position, context.size, *scrollArea, context.alpha, context);
    }

    int getPriority() const override
    {
        return 30; // 滚动条在所有内容渲染之后，裁剪区域弹出之前渲染
    }

private:
    void drawScrollBars(entt::entity entity,
                        const Eigen::Vector2f& pos,
                        const Eigen::Vector2f& size,
                        const components::ScrollArea& scrollArea,
                        float alpha,
                        core::RenderContext& context)
    {
        // 计算可视区域高度（减去垂直内边距）
        float viewportHeight = size.y();
        if (const auto* padding = Registry::TryGet<components::Padding>(entity))
        {
            viewportHeight = std::max(0.0F, size.y() - padding->values.x() - padding->values.z());
        }

        // 简单绘制一个垂直滚动条
        bool hasVerticalScroll =
            (scrollArea.scroll == policies::Scroll::Vertical || scrollArea.scroll == policies::Scroll::Both);
        if (hasVerticalScroll && scrollArea.contentSize.y() > viewportHeight)
        {
            float trackSize = size.y();
            float visibleRatio = viewportHeight / scrollArea.contentSize.y();
            float thumbSize = std::max(20.0f, trackSize * visibleRatio);
            float maxScroll = std::max(0.0f, scrollArea.contentSize.y() - viewportHeight);
            float scrollRatio =
                maxScroll > 0.0f ? std::clamp(scrollArea.scrollOffset.y() / maxScroll, 0.0f, 1.0f) : 0.0f;
            float thumbPos = (trackSize - thumbSize) * scrollRatio;

            // 确保滑块位置不超出轨道
            thumbPos = std::clamp(thumbPos, 0.0f, trackSize - thumbSize);

            // 绘制滑块
            float barWidth = 10.0f;
            Eigen::Vector2f barPos(pos.x() + size.x() - barWidth - 2.0f, pos.y() + thumbPos);
            Eigen::Vector2f barSize(barWidth, thumbSize);

            render::UiPushConstants pushConstants{};
            pushConstants.screen_size[0] = context.screenWidth;
            pushConstants.screen_size[1] = context.screenHeight;
            pushConstants.rect_size[0] = barSize.x();
            pushConstants.rect_size[1] = barSize.y();
            pushConstants.radius[0] = 5.0f; // 左上
            pushConstants.radius[1] = 5.0f; // 右上
            pushConstants.radius[2] = 5.0f; // 右下
            pushConstants.radius[3] = 5.0f; // 左下
            pushConstants.opacity = alpha;

            context.batchManager->beginBatch(m_whiteTexture, context.currentScissor, pushConstants);
            context.batchManager->addRect(barPos, barSize, {0.6f, 0.6f, 0.6f, 0.8f});
        }
    }

private:
    SDL_GPUTexture* m_whiteTexture = nullptr;
};

} // namespace ui::renderers
