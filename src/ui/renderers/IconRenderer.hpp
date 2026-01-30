/**
 * ************************************************************************
 *
 * @file IconRenderer.hpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-30
 * @version 0.1
 * @brief 图标渲染器 - 处理所有图标的渲染

    两种图标：
    - 一种由png jpg 转换的
    - 一种由字体图标 ttf文件转换的
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
#include "../managers/IconManager.hpp"
#include "../api/Utils.hpp"

namespace ui::renderers
{

/**
 * @brief 图标渲染器
 *
 * 负责渲染：
 * - 纹理图标
 * - 字体图标
 */
class IconRenderer : public core::IRenderer
{
public:
    IconRenderer(managers::IconManager& iconManager) : m_iconManager(iconManager) {}

    bool canHandle(entt::entity entity) const override { return Registry::AnyOf<components::Icon>(entity); }

    void collect(entt::entity entity, core::RenderContext& context) override
    {
        if (!context.batchManager)
        {
            return;
        }

        const auto* iconComp = Registry::TryGet<components::Icon>(entity);
        if (!iconComp) return;

        // 根据类型判断是否有有效数据
        if (iconComp->type == policies::IconType::Texture && iconComp->textureId.empty()) return;
        if (iconComp->type == policies::IconType::Font && iconComp->codepoint == 0) return;

        // 计算图标的绘制位置和大小
        Eigen::Vector2f iconDrawPos = context.position;
        // 使用组件定义的尺寸和颜色
        Eigen::Vector2f iconDrawSize = iconComp->size;
        Eigen::Vector4f tint = Eigen::Vector4f(
            iconComp->tintColor.red, iconComp->tintColor.green, iconComp->tintColor.blue, iconComp->tintColor.alpha);

        SDL_GPUTexture* iconTexture = nullptr;
        Eigen::Vector2f uvMin = {0.0f, 0.0f};
        Eigen::Vector2f uvMax = {1.0f, 1.0f};
        Eigen::Vector2f actualIconSize = iconDrawSize;

        if (iconComp->type == policies::IconType::Texture)
        {
            // 纹理图标
            if (auto* textureInfo = m_iconManager.getTextureInfo(iconComp->textureId))
            {
                iconTexture = textureInfo->texture;
                uvMin = textureInfo->uvMin;
                uvMax = textureInfo->uvMax;
                actualIconSize = iconDrawSize.cwiseMin(Eigen::Vector2f(textureInfo->width, textureInfo->height));
            }
            else
            {
                return; // 纹理不存在
            }
        }
        else if (iconComp->type == policies::IconType::Font)
        {
            // 字体图标
            // TODO: FontManager 应该提供获取字体图标纹理的接口，这里暂时简化
            // 需要将字体图标渲染到一张纹理图集上，然后根据字符的UV来绘制
            // 目前 TextTextureCache 只处理普通文本，需要一个 IconFontTextureCache
            // 暂用白色纹理占位
            iconTexture = context.deviceManager->getWhiteTexture();
            if (iconTexture == nullptr) return;

            // 假定字体图标与文本大小一致
            // float iconWidth = context.fontManager->measureTextWidth(iconComp->fontIconName);
            // float iconHeight = context.fontManager->getFontHeight();
            // actualIconSize = {iconWidth, iconHeight};
        }

        if (iconTexture)
        {
            render::UiPushConstants pushConstants{};
            pushConstants.screen_size[0] = context.screenWidth;
            pushConstants.screen_size[1] = context.screenHeight;
            pushConstants.rect_size[0] = actualIconSize.x();
            pushConstants.rect_size[1] = actualIconSize.y();
            pushConstants.opacity = context.alpha;

            context.batchManager->beginBatch(iconTexture, context.currentScissor, pushConstants);
            context.batchManager->addRect(iconDrawPos, actualIconSize, tint, uvMin, uvMax);
        }
    }

    int getPriority() const override
    {
        return 20; // 图标在文本之后渲染
    }

private:
    managers::IconManager& m_iconManager;
};

} // namespace ui::renderers
