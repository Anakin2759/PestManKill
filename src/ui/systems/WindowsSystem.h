/**
 * ************************************************************************
 *
 * @file WindowSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-11 (New)
 * @version 0.1
 * @brief 窗口系统 - 处理应用窗口与ECS根实体的同步
 *
 * 负责将外部窗口的尺寸变化同步到ECS根实体的Size组件，并触发布局更新。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include "src/client/utils/utils.h"             // 包含 Registry
#include "src/client/components/UIComponents.h" // 包含 Size
#include "src/client/components/UITags.h"       // 包含 LayoutDirtyTag

namespace ui::systems
{

/**
 * @brief 窗口同步系统
 *
 * 这是一个响应外部事件而非在主循环中遍历的系统。
 */
class WindowSystem
{
public:
    /**
     * @brief 响应窗口尺寸变化事件，同步ECS根实体的尺寸。
     *
     * 该方法通常由 Application::run() 循环中的 SDL_EVENT_WINDOW_RESIZED 事件直接调用。
     *
     * @param rootEntity ECS的根实体，代表整个应用屏幕。
     * @param newWidth 窗口的新宽度 (像素)。
     * @param newHeight 窗口的新高度 (像素)。
     */
    void onResize(entt::entity rootEntity, int newWidth, int newHeight)
    {
        auto& registry = utils::Registry::getInstance();

        if (!registry.valid(rootEntity))
        {
            // 根实体无效，通常只在初始化阶段发生
            return;
        }

        auto* sizeComp = registry.try_get<components::Size>(rootEntity);

        if (sizeComp)
        {
            // 1. 更新根实体的尺寸
            float newW = static_cast<float>(newWidth);
            float newH = static_cast<float>(newHeight);

            // 避免不必要的更新
            if (sizeComp->width != newW || sizeComp->height != newH)
            {
                sizeComp->width = newW;
                sizeComp->height = newH;

                // 2. 标记根实体为 LayoutDirtyTag
                // 强制 UILayoutSystem 在下一帧运行，重新计算所有子元素的布局
                registry.emplace_or_replace<components::LayoutDirtyTag>(rootEntity);

                // 可选：触发一个 ECS 事件通知其他系统（例如 RenderSystem 可能需要更新投影矩阵）
                // utils::Dispatcher::getInstance().trigger<events::WindowResizeEvent>(newW, newH);
            }
        }
    }

    // WindowSystem 不需要常规的 update(deltaTime) 循环，因为它只响应外部事件。
    // 如果需要处理如最大化/最小化等 ECS 状态，可以增加一个 update() 函数。
};

} // namespace ui::systems