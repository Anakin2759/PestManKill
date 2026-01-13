/**
 * ************************************************************************
 *
 * @file LayoutSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-11 (Finalized)
 * @version 0.4
 * @brief UI布局系统 - 基于 Yoga (Flexbox) 引擎
 *
 * 使用 Facebook Yoga 库实现 Flexbox 布局算法。
 * 负责计算和设置所有UI实体的位置 (Position) 和尺寸 (Size) 组件。
    - 将 ECS 实体树映射为 Yoga 节点树。
    - 根据 Size 和 LayoutInfo 组件设置 Yoga 节点属性。
    - 处理自动居中和填充等布局需求。
    - 计算完成后将结果回写到 Position 和 Size 组件。
    - 支持脏化标记，优化布局计算频率。
    - 对渲染系统无感知，纯粹的布局计算。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cmath>
#include <imgui.h>

// Yoga 布局引擎
#include <yoga/Yoga.h>

#include <utils.h>
#include "src/ui/components/Components.h"
#include "src/ui/components/Tags.h"
#include "src/ui/components/Policies.h"
#include "src/ui/components/Events.h"
#include "src/ui/interface/Isystem.h"

namespace ui::systems
{

/**
 * @brief 基于 Yoga Flexbox 的布局系统
 *
 * 将 ECS 实体树映射到 Yoga 节点树，计算布局后回写到 Position/Size 组件。
 */
class LayoutSystem : public ui::interface::EnableRegister<LayoutSystem>
{
public:
    LayoutSystem() { m_yogaConfig = YGConfigNew(); }

    ~LayoutSystem()
    {
        clearYogaNodes();
        if (m_yogaConfig)
        {
            YGConfigFree(m_yogaConfig);
            m_yogaConfig = nullptr;
        }
    }

    // 禁用拷贝
    LayoutSystem(const LayoutSystem&) = delete;
    LayoutSystem& operator=(const LayoutSystem&) = delete;

    // 自定义移动构造：转移资源所有权
    LayoutSystem(LayoutSystem&& other) noexcept
        : m_yogaConfig(other.m_yogaConfig), m_entityToNode(std::move(other.m_entityToNode)),
          m_rootNodes(std::move(other.m_rootNodes))
    {
        other.m_yogaConfig = nullptr; // 防止源对象析构时释放资源
    }

    // 自定义移动赋值：转移资源所有权
    LayoutSystem& operator=(LayoutSystem&& other) noexcept
    {
        if (this != &other)
        {
            // 释放当前资源
            clearYogaNodes();
            if (m_yogaConfig)
            {
                YGConfigFree(m_yogaConfig);
            }

            // 转移资源
            m_yogaConfig = other.m_yogaConfig;
            m_entityToNode = std::move(other.m_entityToNode);
            m_rootNodes = std::move(other.m_rootNodes);

            // 清空源对象
            other.m_yogaConfig = nullptr;
        }
        return *this;
    }

    void registerHandlersImpl()
    {
        auto& dispatcher = utils::Dispatcher::getInstance();
        dispatcher.sink<ui::events::UpdateLayout>().connect<&LayoutSystem::update>(*this);

        // 注册自动脏化观察器
        auto& registry = utils::Registry::getInstance();
        registry.on_update<components::Size>().connect<&LayoutSystem::onSizeChanged>(*this);
        registry.on_update<components::LayoutInfo>().connect<&LayoutSystem::onLayoutInfoChanged>(*this);
        registry.on_update<components::Padding>().connect<&LayoutSystem::onPaddingChanged>(*this);
        registry.on_update<components::Hierarchy>().connect<&LayoutSystem::onHierarchyChanged>(*this);
    }

    void unregisterHandlersImpl()
    {
        auto& dispatcher = utils::Dispatcher::getInstance();
        dispatcher.sink<ui::events::UpdateLayout>().disconnect<&LayoutSystem::update>(*this);

        auto& registry = utils::Registry::getInstance();
        registry.on_update<components::Size>().disconnect<&LayoutSystem::onSizeChanged>(*this);
        registry.on_update<components::LayoutInfo>().disconnect<&LayoutSystem::onLayoutInfoChanged>(*this);
        registry.on_update<components::Padding>().disconnect<&LayoutSystem::onPaddingChanged>(*this);
        registry.on_update<components::Hierarchy>().disconnect<&LayoutSystem::onHierarchyChanged>(*this);
    }

    /**
     * @brief 每帧更新布局
     */
    void update() noexcept
    {
        auto& registry = utils::Registry::getInstance();

        // LayoutSystem 仅依赖 ECS 组件：画布尺寸来自主画布实体（MainWidgetTag）的 Size。
        const ImVec2 canvas = getCanvasSizeFromEcs(registry);
        const bool canvasValid = (canvas.x > 0.0F && canvas.y > 0.0F);

        // 检查是否有脏节点
        auto dirtyView = registry.view<const components::LayoutDirtyTag>();
        if (dirtyView.begin() == dirtyView.end()) return;

        // 查找所有顶层容器
        std::vector<entt::entity> rootEntities;
        auto view = registry.view<const components::Hierarchy, const components::Position>();
        for (auto entity : view)
        {
            const auto& hierarchy = registry.get<const components::Hierarchy>(entity);
            if (hierarchy.parent == entt::null)
            {
                rootEntities.push_back(entity);
            }
        }

        // 清理上一帧的 Yoga 节点
        clearYogaNodes();

        bool needRetryForCentering = false;

        // 对每个根节点构建 Yoga 树并计算布局
        for (entt::entity root : rootEntities)
        {
            // 构建 Yoga 节点树
            YGNodeRef rootNode = buildYogaTree(registry, root);
            if (!rootNode) continue;
            m_rootNodes.push_back(rootNode); // 保存根节点以便后续释放

            // 3. 获取根容器尺寸（用于布局计算）
            float rootWidth = YGUndefined;
            float rootHeight = YGUndefined;

            if (const auto* sizeComp = registry.try_get<components::Size>(root))
            {
                // 处理 FillParent（屏幕尺寸）
                if (sizeComp->widthPolicy == policies::Size::FillParent)
                {
                    rootWidth = canvasValid ? canvas.x : sizeComp->size.x;
                }
                else
                {
                    rootWidth = sizeComp->size.x;
                }

                if (sizeComp->heightPolicy == policies::Size::FillParent)
                {
                    rootHeight = canvasValid ? canvas.y : sizeComp->size.y;
                }
                else
                {
                    rootHeight = sizeComp->size.y;
                }
            }

            // 4. 计算布局
            YGNodeCalculateLayout(rootNode, rootWidth, rootHeight, YGDirectionLTR);

            // 5. 回写布局结果到 ECS
            applyYogaLayout(registry, root, rootNode, 0.0F, 0.0F);

            // 6. 应用窗口居中策略（针对顶层容器，使用画布尺寸）
            if (canvasValid)
            {
                applyWindowCentering(registry, root, canvas.x, canvas.y);
            }
            else
            {
                // 如果当前根节点需要居中，但画布尺寸尚未准备好，则保留脏标记，下一帧自动重算。
                if (wantsWindowCentering(registry, root))
                {
                    needRetryForCentering = true;
                }
            }
        }

        // 清除所有脏标记。
        // 若画布尺寸尚未准备好且存在需要居中的根节点，则保留脏标记以便下一帧重算。
        if (!needRetryForCentering)
        {
            for (auto entity : dirtyView)
            {
                registry.remove<components::LayoutDirtyTag>(entity);
            }
        }
    }

    ImVec2 getCanvasSizeFromEcs(entt::registry& registry) const
    {
        auto view = registry.view<const components::MainWidgetTag, const components::Size>();
        for (auto entity : view)
        {
            const auto& size = registry.get<const components::Size>(entity);
            return size.size;
        }
        return ImVec2(0.0F, 0.0F);
    }

    bool wantsWindowCentering(entt::registry& registry, entt::entity root) const
    {
        auto* pos = registry.try_get<components::Position>(root);
        auto* size = registry.try_get<components::Size>(root);
        if (pos == nullptr || size == nullptr) return false;

        // 默认：位置是 (0,0) 且尺寸有效，则视为需要自动居中
        bool wantCenter = (pos->value.x == 0.0F && pos->value.y == 0.0F && size->size.x > 0.0F && size->size.y > 0.0F);

        if (const auto* posPolicy = registry.try_get<policies::PositionPolicy>(root))
        {
            wantCenter = (posPolicy->horizontal == policies::PositionPolicy::Center ||
                          posPolicy->vertical == policies::PositionPolicy::Center);

            if (posPolicy->horizontal == policies::PositionPolicy::Fixed &&
                posPolicy->vertical == policies::PositionPolicy::Fixed)
            {
                wantCenter = false;
            }
        }

        return wantCenter;
    }

private:
    YGConfigRef m_yogaConfig = nullptr;
    std::unordered_map<entt::entity, YGNodeRef> m_entityToNode;
    std::vector<YGNodeRef> m_rootNodes; // 保存所有根节点以便正确释放

    // ===================== Yoga 节点管理 =====================

    void clearYogaNodes()
    {
        // 释放所有根节点，Yoga 会自动递归释放其子节点
        for (YGNodeRef rootNode : m_rootNodes)
        {
            if (rootNode)
            {
                YGNodeFreeRecursive(rootNode);
            }
        }
        m_rootNodes.clear();
        m_entityToNode.clear();
    }

    YGNodeRef createYogaNode() { return YGNodeNewWithConfig(m_yogaConfig); }

    /**
     * @brief 递归构建 Yoga 节点树
     */
    YGNodeRef buildYogaTree(entt::registry& registry, entt::entity entity)
    {
        YGNodeRef node = createYogaNode();
        m_entityToNode[entity] = node;

        // 配置节点样式
        configureYogaNode(registry, entity, node);

        // 递归处理子节点
        const auto* hierarchy = registry.try_get<components::Hierarchy>(entity);
        if (hierarchy && !hierarchy->children.empty())
        {
            uint32_t childIndex = 0;
            for (entt::entity child : hierarchy->children)
            {
                if (!registry.all_of<components::Position, components::Size>(child)) continue;

                YGNodeRef childNode = buildYogaTree(registry, child);
                if (childNode)
                {
                    YGNodeInsertChild(node, childNode, childIndex++);
                }
            }
        }

        return node;
    }

    /**
     * @brief 配置单个 Yoga 节点的样式
     */
    void configureYogaNode(entt::registry& registry, entt::entity entity, YGNodeRef node)
    {
        // 1. 布局方向 (Flex Direction)
        if (const auto* layoutInfo = registry.try_get<components::LayoutInfo>(entity))
        {
            if (layoutInfo->direction == policies::LayoutDirection::VERTICAL)
            {
                YGNodeStyleSetFlexDirection(node, YGFlexDirectionColumn);
            }
            else
            {
                YGNodeStyleSetFlexDirection(node, YGFlexDirectionRow);
            }

            // 间距通过 gap 实现
            YGNodeStyleSetGap(node, YGGutterAll, layoutInfo->spacing);
        }

        // 2. 内边距 (Padding)
        if (const auto* padding = registry.try_get<components::Padding>(entity))
        {
            YGNodeStyleSetPadding(node, YGEdgeTop, padding->values.x);
            YGNodeStyleSetPadding(node, YGEdgeRight, padding->values.y);
            YGNodeStyleSetPadding(node, YGEdgeBottom, padding->values.w);
            YGNodeStyleSetPadding(node, YGEdgeLeft, padding->values.z);
        }

        // 3. 尺寸 (Size)
        if (const auto* sizeComp = registry.try_get<components::Size>(entity))
        {
            // 宽度策略
            if (sizeComp->widthPolicy == policies::Size::Fixed)
            {
                YGNodeStyleSetWidth(node, sizeComp->size.x);
            }
            else if (sizeComp->widthPolicy == policies::Size::FillParent)
            {
                YGNodeStyleSetWidthPercent(node, 100.0f);
            }
            else if (sizeComp->widthPolicy == policies::Size::Auto)
            {
                YGNodeStyleSetWidthAuto(node);
            }

            // 高度策略
            if (sizeComp->heightPolicy == policies::Size::Fixed)
            {
                YGNodeStyleSetHeight(node, sizeComp->size.y);
            }
            else if (sizeComp->heightPolicy == policies::Size::FillParent)
            {
                YGNodeStyleSetHeightPercent(node, 100.0f);
            }
            else if (sizeComp->heightPolicy == policies::Size::Auto)
            {
                YGNodeStyleSetHeightAuto(node);
            }

            // 非 autoSize 的固定尺寸
            if (!sizeComp->autoSize && sizeComp->size.x > 0 && sizeComp->size.y > 0)
            {
                if (sizeComp->widthPolicy == policies::Size::Auto)
                {
                    YGNodeStyleSetWidth(node, sizeComp->size.x);
                }
                if (sizeComp->heightPolicy == policies::Size::Auto)
                {
                    YGNodeStyleSetHeight(node, sizeComp->size.y);
                }
            }

            // 最小/最大尺寸约束
            if (sizeComp->minSize.x > 0) YGNodeStyleSetMinWidth(node, sizeComp->minSize.x);
            if (sizeComp->minSize.y > 0) YGNodeStyleSetMinHeight(node, sizeComp->minSize.y);
            if (sizeComp->maxSize.x < FLT_MAX) YGNodeStyleSetMaxWidth(node, sizeComp->maxSize.x);
            if (sizeComp->maxSize.y < FLT_MAX) YGNodeStyleSetMaxHeight(node, sizeComp->maxSize.y);
        }

        // 4. Spacer 处理 (flexGrow)
        if (registry.any_of<components::SpacerTag>(entity))
        {
            if (const auto* spacer = registry.try_get<components::Spacer>(entity))
            {
                YGNodeStyleSetFlexGrow(node, static_cast<float>(spacer->stretchFactor));
                YGNodeStyleSetFlexShrink(node, 0.0f);
            }
        }

        // 5. 对齐方式
        if (const auto* alignComp = registry.try_get<policies::Alignment>(entity))
        {
            const uint8_t alignment = static_cast<uint8_t>(*alignComp);

            // 主轴对齐 (justify-content 由父容器设置)
            // 交叉轴自身对齐 (align-self)
            if (alignment & static_cast<uint8_t>(policies::Alignment::VCENTER))
            {
                YGNodeStyleSetAlignSelf(node, YGAlignCenter);
            }
            else if (alignment & static_cast<uint8_t>(policies::Alignment::TOP))
            {
                YGNodeStyleSetAlignSelf(node, YGAlignFlexStart);
            }
            else if (alignment & static_cast<uint8_t>(policies::Alignment::BOTTOM))
            {
                YGNodeStyleSetAlignSelf(node, YGAlignFlexEnd);
            }
        }

        // 6. 容器默认居中对齐子元素
        if (registry.any_of<components::LayoutInfo>(entity))
        {
            YGNodeStyleSetAlignItems(node, YGAlignCenter);          // 交叉轴居中
            YGNodeStyleSetJustifyContent(node, YGJustifyFlexStart); // 主轴从头排列
        }

        // 7. 文本/按钮等叶子节点：使用 Size 组件中已有的尺寸
        // 注意：不能在这里调用 ImGui::CalcTextSize，因为可能在 ImGui 帧外执行
        if (!registry.any_of<components::LayoutInfo>(entity))
        {
            // 叶子节点：如果没有设置固定尺寸，给一个合理的默认值
            if (const auto* sizeComp = registry.try_get<components::Size>(entity))
            {
                if (sizeComp->autoSize)
                {
                    // 自动尺寸的叶子节点，使用默认最小尺寸
                    float defaultWidth = 100.0f;
                    float defaultHeight = 20.0f;

                    // 如果有文本，根据文本长度估算
                    if (const auto* text = registry.try_get<components::Text>(entity))
                    {
                        if (!text->content.empty())
                        {
                            // 粗略估算：每个字符约 8 像素宽，高度 20 像素
                            defaultWidth = static_cast<float>(text->content.length()) * 8.0f + 10.0f;
                            defaultHeight = 20.0f;
                        }
                    }

                    YGNodeStyleSetMinWidth(node, defaultWidth);
                    YGNodeStyleSetMinHeight(node, defaultHeight);
                }
            }
        }
    }

    /**
     * @brief 递归应用 Yoga 布局结果到 ECS 组件
     */
    void applyYogaLayout(entt::registry& registry, entt::entity entity, YGNodeRef node, float parentX, float parentY)
    {
        if (!node) return;

        // 获取 Yoga 计算的布局结果
        float left = YGNodeLayoutGetLeft(node);
        float top = YGNodeLayoutGetTop(node);
        float width = YGNodeLayoutGetWidth(node);
        float height = YGNodeLayoutGetHeight(node);

        // 安全检查：跳过无效值
        if (std::isnan(left)) left = 0.0f;
        if (std::isnan(top)) top = 0.0f;

        // 回写到 Position 组件（相对于父容器）
        if (auto* pos = registry.try_get<components::Position>(entity))
        {
            pos->value.x = left;
            pos->value.y = top;
        }

        // 回写到 Size 组件（仅当值有效时）
        if (auto* size = registry.try_get<components::Size>(entity))
        {
            if (!std::isnan(width) && width > 0) size->size.x = width;
            if (!std::isnan(height) && height > 0) size->size.y = height;
        }

        // 递归处理子节点
        const uint32_t childCount = YGNodeGetChildCount(node);
        const auto* hierarchy = registry.try_get<components::Hierarchy>(entity);

        if (hierarchy && childCount > 0)
        {
            uint32_t yogaChildIndex = 0;
            for (entt::entity child : hierarchy->children)
            {
                if (!registry.all_of<components::Position, components::Size>(child)) continue;

                if (yogaChildIndex < childCount)
                {
                    YGNodeRef childNode = YGNodeGetChild(node, yogaChildIndex);
                    applyYogaLayout(registry, child, childNode, left, top);
                    yogaChildIndex++;
                }
            }
        }
    }

    // ===================== 窗口居中策略 =====================

    /**
     * @brief 应用窗口居中策略
     *
     * 对于顶层容器（窗口/菜单），如果没有设置固定位置，则自动居中显示。
     * 如果 Position 已有非零值，则优先使用该位置。
     */
    void applyWindowCentering(entt::registry& registry, entt::entity root, float screenWidth, float screenHeight)
    {
        auto* pos = registry.try_get<components::Position>(root);
        auto* size = registry.try_get<components::Size>(root);
        if (!pos || !size) return;

        // 检查是否有 PositionPolicy 组件来决定居中策略
        // 默认：如果位置是 (0,0) 且尺寸有效，则居中
        bool shouldCenter = (pos->value.x == 0.0f && pos->value.y == 0.0f);

        // 如果有明确的位置策略组件，检查是否要求居中
        if (const auto* posPolicy = registry.try_get<policies::PositionPolicy>(root))
        {
            shouldCenter = (posPolicy->horizontal == policies::PositionPolicy::Center ||
                            posPolicy->vertical == policies::PositionPolicy::Center);

            // 如果策略是固定位置，不居中
            if (posPolicy->horizontal == policies::PositionPolicy::Fixed &&
                posPolicy->vertical == policies::PositionPolicy::Fixed)
            {
                shouldCenter = false;
            }
        }

        if (shouldCenter && size->size.x > 0 && size->size.y > 0)
        {
            pos->value.x = (screenWidth - size->size.x) / 2.0f;
            pos->value.y = (screenHeight - size->size.y) / 2.0f;
        }
    }

    // ===================== 脏标记回调 =====================

    void onSizeChanged(entt::registry& registry, entt::entity entity) { markEntityAndParentsDirty(registry, entity); }

    void onLayoutInfoChanged(entt::registry& registry, entt::entity entity)
    {
        registry.emplace_or_replace<components::LayoutDirtyTag>(entity);
    }

    void onPaddingChanged(entt::registry& registry, entt::entity entity)
    {
        registry.emplace_or_replace<components::LayoutDirtyTag>(entity);
    }

    void onHierarchyChanged(entt::registry& registry, entt::entity entity)
    {
        if (registry.any_of<components::LayoutInfo>(entity))
        {
            registry.emplace_or_replace<components::LayoutDirtyTag>(entity);
        }
    }

    void markEntityAndParentsDirty(entt::registry& registry, entt::entity entity)
    {
        entt::entity current = entity;
        while (current != entt::null && registry.valid(current))
        {
            registry.emplace_or_replace<components::LayoutDirtyTag>(current);
            const auto* hierarchy = registry.try_get<components::Hierarchy>(current);
            current = hierarchy ? hierarchy->parent : entt::null;
        }
    }
};

} // namespace ui::systems