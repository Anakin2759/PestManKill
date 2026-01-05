/**
 * ************************************************************************
 *
 * @file LayoutSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-11 (Finalized)
 * @version 0.3
 * @brief UI布局系统 事件驱动
 *
 * 负责计算和设置所有UI实体的位置 (Position) 和尺寸 (Size) 组件。
 * 新增：拉伸因子 (Spacer) 和 对齐 (Alignment) 的处理逻辑。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include <vector>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <functional> // For std::function in traversal

#include <utils.h>
#include "src/ui/components/Components.h"
#include "src/ui/components/Tags.h"
#include "src/ui/components/Define.h"
#include "src/ui/components/Events.h"
#include "src/ui/interface/Isystem.h"

namespace ui::systems
{

class LayoutSystem : public ui::interface::EnableRegister<LayoutSystem>
{
public:
    void registerHandlersImpl()
    {
        auto& dispatcher = utils::Dispatcher::getInstance();
        dispatcher.sink<ui::events::UpdateLayout>().connect<&LayoutSystem::update>(*this);
    }
    void unregisterHandlersImpl()
    {
        auto& dispatcher = utils::Dispatcher::getInstance();
        dispatcher.sink<ui::events::UpdateLayout>().disconnect<&LayoutSystem::update>(*this);
    }

    /**
     * @brief 每帧更新布局
     */
    void update() noexcept
    {
        auto& registry = utils::Registry::getInstance();

        // 1. 查找所有需要更新的容器
        auto dirtyView = registry.view<const components::LayoutDirtyTag>();
        if (dirtyView.begin() == dirtyView.end()) return;

        // 2. 拓扑排序：从叶子节点到根节点 (Post-order traversal)
        std::vector<entt::entity> sortedEntities = getLayoutSortOrder();

        // 3. 核心布局计算：迭代处理已排序的实体
        for (auto entity : sortedEntities)
        {
            if (registry.any_of<components::LayoutDirtyTag>(entity) && registry.any_of<components::LayoutInfo>(entity))
            {
                calculateLayout(entity);
                registry.remove<components::LayoutDirtyTag>(entity);
            }
        }
    }

private:
    /**
     * @brief 获取布局计算的拓扑排序顺序 (后序遍历)
     * @return std::vector<entt::entity> 排序后的实体列表
     */
    std::vector<entt::entity> getLayoutSortOrder()
    {
        std::vector<entt::entity> result;
        std::vector<entt::entity> rootEntities;

        auto& registry = utils::Registry::getInstance();

        // 查找所有顶层元素
        auto view = registry.view<const components::Hierarchy, const components::Position>();
        for (auto entity : view)
        {
            const auto& hierarchy = registry.get<const components::Hierarchy>(entity);
            if (hierarchy.parent == entt::null)
            {
                rootEntities.push_back(entity);
            }
        }

        // 迭代 DFS 实现后序遍历
        std::function<void(entt::entity)> traverse = [&](entt::entity entity)
        {
            const auto* hierarchy = registry.try_get<components::Hierarchy>(entity);
            if (hierarchy)
            {
                for (entt::entity child : hierarchy->children)
                {
                    traverse(child);
                }
            }
            // 只有容器实体才需要计算布局
            if (registry.any_of<components::LayoutInfo>(entity))
            {
                result.push_back(entity);
            }
        };

        for (entt::entity root : rootEntities)
        {
            traverse(root);
        }

        return result;
    }

    /**
     * @brief 核心布局计算函数
     * @param registry EnTT 注册表
     * @param containerEntity 当前布局容器实体
     */
    void calculateLayout(entt::entity containerEntity)
    {
        auto& registry = utils::Registry::getInstance();
        const auto& layout = registry.get<const components::LayoutInfo>(containerEntity);
        const auto& hierarchy = registry.get<const components::Hierarchy>(containerEntity);

        const auto* paddingComp = registry.try_get<components::Padding>(containerEntity);
        const ImVec4 padding = paddingComp ? paddingComp->values : ImVec4(0, 0, 0, 0);

        auto& containerSize = registry.get<components::Size>(containerEntity);

        // ----------------------------------------------------
        // I. 预计算：测量固定尺寸和总拉伸因子
        // ----------------------------------------------------
        float fixedSpace = 0.0f;   // 固定尺寸子元素 + 间距的总和
        uint32_t totalStretch = 0; // 所有 Spacer 的拉伸因子总和

        for (entt::entity childEntity : hierarchy.children)
        {
            if (!registry.all_of<components::Position, components::Size>(childEntity)) continue;

            bool hasSpacer = registry.all_of<components::SpacerTag>(childEntity);

            if (hasSpacer)
            {
                const auto& spacerComp = registry.get<const components::Spacer>(childEntity);
                totalStretch += spacerComp.stretchFactor;
            }
            else
            {
                const auto& childSize = registry.get<const components::Size>(childEntity);

                // 累加固定尺寸元素占用的主要轴空间
                if (layout.direction == components::LayoutDirection::HORIZONTAL)
                {
                    fixedSpace += childSize.size.x;
                }
                else // VERTICAL
                {
                    fixedSpace += childSize.size.y;
                }

                // 累加间距
                fixedSpace += layout.spacing;
            }
        }
        // 减去末尾多余的 spacing (如果子元素列表不为空)
        if (!hierarchy.children.empty() && fixedSpace > 0)
        {
            fixedSpace -= layout.spacing;
        }

        // ----------------------------------------------------
        // II. 计算容器自身尺寸 (AutoSize)
        // ----------------------------------------------------

        // 可用空间 = 容器尺寸 - 填充
        float availableWidth = containerSize.size.x - (padding.y + padding.z);  // Left + Right
        float availableHeight = containerSize.size.y - (padding.x + padding.w); // Top + Bottom

        // 如果容器是自适应尺寸，则根据内容尺寸设定
        if (containerSize.autoSize)
        {
            // 假设所有 Spacer 尺寸为 0，计算最小内容尺寸
            float minContentSpace = fixedSpace;

            if (layout.direction == components::LayoutDirection::HORIZONTAL)
            {
                containerSize.size.x = minContentSpace + padding.y + padding.z;
            }
            else // VERTICAL
            {
                containerSize.size.y = minContentSpace + padding.x + padding.w;
            }

            // 重新计算可用空间 (此时可用空间等于内容尺寸)
            availableWidth = containerSize.size.x - (padding.y + padding.z);
            availableHeight = containerSize.size.y - (padding.x + padding.w);
        }

        // ----------------------------------------------------
        // III. 拉伸计算和最终位置分配
        // ----------------------------------------------------

        // 1. 计算剩余可用空间 (用于 Spacer)
        float remainingSpace = 0.0f;

        if (layout.direction == components::LayoutDirection::HORIZONTAL)
        {
            remainingSpace = availableWidth - fixedSpace;
        }
        else
        {
            remainingSpace = availableHeight - fixedSpace;
        }

        // 2. 计算每个拉伸因子分配的空间
        float spacePerStretch = (totalStretch > 0 && remainingSpace > 0) ? remainingSpace / (float)totalStretch : 0.0f;

        // 3. 最终分配位置和尺寸
        float currentX = padding.y; // Start at Left Padding
        float currentY = padding.x; // Start at Top Padding

        // 获取容器的总宽度和高度 (用于对齐)
        float containerInnerWidth = containerSize.size.x - (padding.y + padding.z);
        float containerInnerHeight = containerSize.size.y - (padding.x + padding.w);

        // 重新遍历子元素进行位置设置
        for (entt::entity childEntity : hierarchy.children)
        {
            if (!registry.all_of<components::Position, components::Size>(childEntity)) continue;

            auto& childPos = registry.get<components::Position>(childEntity);
            auto& childSize = registry.get<components::Size>(childEntity);

            // 计算 Spacer 尺寸
            float spacerLength = 0.0f;
            if (registry.any_of<components::SpacerTag>(childEntity))
            {
                const auto& spacerComp = registry.get<const components::Spacer>(childEntity);
                spacerLength = spacerComp.stretchFactor * spacePerStretch;
            }

            if (layout.direction == components::LayoutDirection::HORIZONTAL)
            {
                // A. 设置 X 轴位置
                childPos.value.x = currentX;

                // B. 设置 Y 轴对齐 (垂直对齐)
                applyVerticalAlignment(registry, childEntity, childPos, childSize, containerInnerHeight, currentY);

                // C. 推进位置
                if (registry.any_of<components::SpacerTag>(childEntity))
                {
                    // Spacer 只推进位置
                    currentX += spacerLength;
                }
                else
                {
                    // 非 Spacer 推进其固定尺寸
                    currentX += childSize.size.x;
                }
                currentX += layout.spacing;
            }
            else // VERTICAL
            {
                // A. 设置 Y 轴位置
                childPos.value.y = currentY;

                // B. 设置 X 轴对齐 (水平对齐)
                applyHorizontalAlignment(registry, childEntity, childPos, childSize, containerInnerWidth, currentX);

                // C. 推进位置
                if (registry.any_of<components::SpacerTag>(childEntity))
                {
                    // Spacer 只推进位置
                    currentY += spacerLength;
                }
                else
                {
                    // 非 Spacer 推进其固定尺寸
                    currentY += childSize.size.y;
                }
                currentY += layout.spacing;
            }
        }
    }

    // =======================================================
    // 辅助函数：对齐处理
    // =======================================================

    /**
     * @brief 应用垂直对齐 (用于水平布局)
     * @param pos 子元素 Position
     * @param size 子元素 Size
     * @param containerInnerHeight 父容器内部高度 (Padding后)
     * @param containerInnerYStart 父容器内部 Y 轴起始位置 (Top Padding)
     */
    void applyVerticalAlignment(entt::registry& registry,
                                entt::entity childEntity,
                                components::Position& pos,
                                const components::Size& size,
                                float containerInnerHeight,
                                float containerInnerYStart)
    {
        // Alignment 组件存储在子元素实体上
        const auto* alignmentComp = registry.try_get<components::Alignment>(childEntity);
        const uint8_t alignment = alignmentComp ? static_cast<uint8_t>(*alignmentComp) : 0;

        // 默认是从 Top Padding 开始，即 Top 贴边
        if (alignment & static_cast<uint8_t>(components::Alignment::VCENTER))
        {
            // 垂直居中 = Y 轴起始位置 + (容器高度 - 自身高度) / 2
            pos.value.y = containerInnerYStart + (containerInnerHeight - size.size.y) / 2.0f;
        }
        else if (alignment & static_cast<uint8_t>(components::Alignment::BOTTOM))
        {
            // 底部对齐 = Y 轴起始位置 + 容器高度 - 自身高度
            pos.value.y = containerInnerYStart + containerInnerHeight - size.size.y;
        }
        // 否则默认为 TOP 对齐，pos.y 已经设置为 containerInnerYStart
    }

    /**
     * @brief 应用水平对齐 (用于垂直布局)
     * @param pos 子元素 Position
     * @param size 子元素 Size
     * @param containerInnerWidth 父容器内部宽度 (Padding后)
     * @param containerInnerXStart 父容器内部 X 轴起始位置 (Left Padding)
     */
    void applyHorizontalAlignment(components::Position& pos,
                                  const components::Size& size,
                                  float containerInnerWidth,
                                  float containerInnerXStart)
    {
        // 假设 Alignment 组件存储在子元素上
        const auto& registry = utils::Registry::getInstance();
        (void)registry;
        (void)pos;
        (void)size;
        (void)containerInnerWidth;
        (void)containerInnerXStart;
        // 旧实现有 bug（把 Position 当 entity）。新实现见下方重载。
    }

    void applyHorizontalAlignment(entt::registry& registry,
                                  entt::entity childEntity,
                                  components::Position& pos,
                                  const components::Size& size,
                                  float containerInnerWidth,
                                  float containerInnerXStart)
    {
        const auto* alignmentComp = registry.try_get<components::Alignment>(childEntity);
        const uint8_t alignment = alignmentComp ? static_cast<uint8_t>(*alignmentComp) : 0;

        // 默认是从 Left Padding 开始，即 LEFT 贴边
        if (alignment & static_cast<uint8_t>(components::Alignment::HCENTER))
        {
            // 水平居中 = X 轴起始位置 + (容器宽度 - 自身宽度) / 2
            pos.value.x = containerInnerXStart + (containerInnerWidth - size.size.x) / 2.0f;
        }
        else if (alignment & static_cast<uint8_t>(components::Alignment::RIGHT))
        {
            // 右对齐 = X 轴起始位置 + 容器宽度 - 自身宽度
            pos.value.x = containerInnerXStart + containerInnerWidth - size.size.x;
        }
        // 否则默认为 LEFT 对齐，pos.x 已经设置为 containerInnerXStart
    }
};

} // namespace ui::systems