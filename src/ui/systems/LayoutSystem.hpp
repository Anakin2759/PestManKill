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
#include <cmath>

// Yoga 布局引擎
#include <yoga/Yoga.h>

#include "common/Components.hpp"
#include "common/Tags.hpp"
#include "common/Policies.hpp"
#include "common/Events.hpp"
#include "interface/Isystem.hpp"
#include "singleton/Registry.hpp"
#include "singleton/Dispatcher.hpp" // 包含 Dispatcher
#include "singleton/Logger.hpp"
#include "traits/ComponentsTraits.hpp"
#include "traits/PoliciesTraits.hpp"

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
    LayoutSystem() : m_yogaConfig(YGConfigNew()) {}

    ~LayoutSystem()
    {
        clearYogaNodes();
        if (m_yogaConfig != nullptr)
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
            if (m_yogaConfig != nullptr)
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
        Dispatcher::Sink<events::UpdateLayout>().connect<&LayoutSystem::update>(*this);

        // 注册自动脏化观察器

        Registry::OnUpdate<components::Size>().connect<&LayoutSystem::onSizeChanged>(*this);
        Registry::OnUpdate<components::LayoutInfo>().connect<&LayoutSystem::onLayoutInfoChanged>(*this);
        Registry::OnUpdate<components::Padding>().connect<&LayoutSystem::onPaddingChanged>(*this);
        Registry::OnUpdate<components::Hierarchy>().connect<&LayoutSystem::onHierarchyChanged>(*this);
    }

    void unregisterHandlersImpl()
    {
        Dispatcher::Sink<events::UpdateLayout>().disconnect<&LayoutSystem::update>(*this);

        Registry::OnUpdate<components::Size>().disconnect<&LayoutSystem::onSizeChanged>(*this);
        Registry::OnUpdate<components::LayoutInfo>().disconnect<&LayoutSystem::onLayoutInfoChanged>(*this);
        Registry::OnUpdate<components::Padding>().disconnect<&LayoutSystem::onPaddingChanged>(*this);
        Registry::OnUpdate<components::Hierarchy>().disconnect<&LayoutSystem::onHierarchyChanged>(*this);
    }

    /**
     * @brief 每帧更新布局
     */
    void update() noexcept
    {
        // LayoutSystem 仅依赖 ECS 组件：画布尺寸来自主画布实体（MainWidgetTag）的 Size。
        const Vec2 canvas = getCanvasSizeFromEcs();
        const bool canvasValid = (canvas.x() > 0.0F && canvas.y() > 0.0F);

        // 检查是否有脏节点
        auto dirtyView = Registry::View<components::LayoutDirtyTag>();
        if (dirtyView.begin() == dirtyView.end()) return;

        // 查找所有顶层容器
        std::vector<entt::entity> rootEntities;
        auto view = Registry::View<components::Hierarchy, components::Position>();
        for (auto entity : view)
        {
            const auto& hierarchy = Registry::Get<components::Hierarchy>(entity);
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
            YGNodeRef rootNode = buildYogaTree(root);
            if (rootNode == nullptr) continue;
            m_rootNodes.push_back(rootNode); // 保存根节点以便后续释放

            // 3. 获取根容器尺寸（用于布局计算）
            float rootWidth = YGUndefined;
            float rootHeight = YGUndefined;

            if (const auto* sizeComp = Registry::TryGet<components::Size>(root))
            {
                // 处理 FillParent（屏幕尺寸）
                if (policies::HasFlag(sizeComp->sizePolicy, policies::Size::HFill))
                {
                    rootWidth = canvasValid ? canvas.x() : sizeComp->size.x();
                }
                else
                {
                    rootWidth = sizeComp->size.x();
                }

                if (policies::HasFlag(sizeComp->sizePolicy, policies::Size::VFill))
                {
                    rootHeight = canvasValid ? canvas.y() : sizeComp->size.y();
                }
                else
                {
                    rootHeight = sizeComp->size.y();
                }
            }

            // 4. 计算布局
            YGNodeCalculateLayout(rootNode, rootWidth, rootHeight, YGDirectionLTR);

            // 5. 回写布局结果到 ECS
            applyYogaLayout(root, rootNode, 0.0F, 0.0F);

            // 6. 应用窗口居中策略（针对顶层容器，使用画布尺寸）
            if (canvasValid)
            {
                applyWindowCentering(root, canvas.x(), canvas.y());
            }
            else
            {
                // 如果当前根节点需要居中，但画布尺寸尚未准备好，则保留脏标记，下一帧自动重算。
                if (wantsWindowCentering(root))
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
                Registry::Remove<components::LayoutDirtyTag>(entity);
            }
        }
    }

    Vec2 getCanvasSizeFromEcs() const // NOLINT(readability-convert-member-functions-to-static)
    {
        auto view = Registry::View<components::MainWidgetTag, components::Size>();
        // NOLINTNEXTLINE
        for (auto entity : view)
        {
            const auto& size = Registry::Get<components::Size>(entity);
            return size.size;
        }
        return {0.0F, 0.0F};
    }

    bool wantsWindowCentering(entt::entity root) const
    {
        auto* pos = Registry::TryGet<components::Position>(root);
        auto* size = Registry::TryGet<components::Size>(root);
        if (pos == nullptr || size == nullptr) return false;

        // 默认：位置是 (0,0) 且尺寸有效，则视为需要自动居中
        bool wantCenter =
            (pos->value.x() == 0.0F && pos->value.y() == 0.0F && size->size.x() > 0.0F && size->size.y() > 0.0F);

        if (auto* pos = Registry::TryGet<components::Position>(root))
        {
            const bool hCenter = policies::HasFlag(pos->positionPolicy, policies::Position::HCenter);
            const bool vCenter = policies::HasFlag(pos->positionPolicy, policies::Position::VCenter);

            if (hCenter || vCenter)
            {
                wantCenter = true;
            }

            const bool hFixed = policies::HasFlag(pos->positionPolicy, policies::Position::HFixed);
            const bool vFixed = policies::HasFlag(pos->positionPolicy, policies::Position::VFixed);

            if (hFixed && vFixed)
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
            if (rootNode != nullptr)
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
    YGNodeRef buildYogaTree(entt::entity entity)
    {
        YGNodeRef node = createYogaNode();
        m_entityToNode[entity] = node;

        // 配置节点样式
        configureYogaNode(entity, node);

        // 递归处理子节点
        const auto* hierarchy = Registry::TryGet<components::Hierarchy>(entity);
        if (hierarchy != nullptr && !hierarchy->children.empty())
        {
            uint32_t childIndex = 0;
            for (entt::entity child : hierarchy->children)
            {
                // Spacer 没有 Size 组件，但仍需参与布局
                const bool isSpacer = Registry::AnyOf<components::SpacerTag>(child);
                const bool hasLayoutComponents = Registry::AllOf<components::Position, components::Size>(child);

                if (!isSpacer && !hasLayoutComponents) continue;

                YGNodeRef childNode = buildYogaTree(child);
                if (childNode != nullptr)
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
    void configureYogaNode(entt::entity entity, YGNodeRef node)
    {
        // 1. 布局方向 (Flex Direction)
        if (const auto* layoutInfo = Registry::TryGet<components::LayoutInfo>(entity))
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
        if (const auto* padding = Registry::TryGet<components::Padding>(entity))
        {
            YGNodeStyleSetPadding(node, YGEdgeTop, padding->values.x());
            YGNodeStyleSetPadding(node, YGEdgeRight, padding->values.y());
            YGNodeStyleSetPadding(node, YGEdgeBottom, padding->values.w());
            YGNodeStyleSetPadding(node, YGEdgeLeft, padding->values.z());
        }

        // 3. Spacer 处理 (优先级最高，跳过后续 Size 配置)
        // Spacer 是纯弹性元素，大小完全由 flexGrow 决定，在其他固定尺寸组件布局后填充剩余空间
        if (Registry::AnyOf<components::SpacerTag>(entity))
        {
            const auto* spacer = Registry::TryGet<components::Spacer>(entity);
            const float stretchFactor = spacer != nullptr ? static_cast<float>(spacer->stretchFactor) : 1.0F;

            YGNodeStyleSetFlexGrow(node, stretchFactor);
            // 允许收缩，防止在空间不足时撑开容器
            YGNodeStyleSetFlexShrink(node, 1.0F);
            // flexBasis 为 0，确保 Spacer 初始大小为 0，完全依赖 flexGrow 分配空间
            YGNodeStyleSetFlexBasis(node, 0.0F);
            // 最小尺寸为 0，不占用任何固定空间
            YGNodeStyleSetMinWidth(node, 0.0F);
            YGNodeStyleSetMinHeight(node, 0.0F);
            // Spacer 不设置固定宽高，跳过后续 Size 配置
            return;
        }

        // 4. 尺寸 (Size) - 仅对非 Spacer 实体生效
        if (const auto* sizeComp = Registry::TryGet<components::Size>(entity))
        {
            // 1. 获取父容器布局方向 (用于判定主轴/交叉轴)
            policies::LayoutDirection parentDir = policies::LayoutDirection::VERTICAL; // Yoga 默认是 Vertical
            if (const auto* hierarchy = Registry::TryGet<components::Hierarchy>(entity))
            {
                if (hierarchy->parent != entt::null)
                {
                    if (const auto* parentLayout = Registry::TryGet<components::LayoutInfo>(hierarchy->parent))
                    {
                        parentDir = parentLayout->direction;
                    }
                }
            }

            const bool isRow = (parentDir == policies::LayoutDirection::HORIZONTAL);

            // 提取策略标志
            const bool wFill = policies::HasFlag(sizeComp->sizePolicy, policies::Size::HFill);
            const bool wFixed = policies::HasFlag(sizeComp->sizePolicy, policies::Size::HFixed);
            const bool wAuto = policies::HasFlag(sizeComp->sizePolicy, policies::Size::HAuto);
            const bool wPct = policies::HasFlag(sizeComp->sizePolicy, policies::Size::HPercentage);

            const bool hFill = policies::HasFlag(sizeComp->sizePolicy, policies::Size::VFill);
            const bool hFixed = policies::HasFlag(sizeComp->sizePolicy, policies::Size::VFixed);
            const bool hAuto = policies::HasFlag(sizeComp->sizePolicy, policies::Size::VAuto);
            const bool hPct = policies::HasFlag(sizeComp->sizePolicy, policies::Size::VPercentage);

            // 2. 主轴行为 (Main Axis) -> FlexGrow
            // Row布局的主轴是Width，Col布局的主轴是Height
            const bool mainAxisFill = (isRow && wFill) || (!isRow && hFill);

            if (mainAxisFill)
            {
                YGNodeStyleSetFlexGrow(node, 1.0F);
                YGNodeStyleSetFlexShrink(node, 1.0F);
                YGNodeStyleSetFlexBasis(node, 0.0F); // 推荐设置：让 flexGrow 完全接管空间分配
            }
            else
            {
                YGNodeStyleSetFlexGrow(node, 0.0F);
                // 如果主轴是 Fixed，则不收缩(Shrink=0)；否则默认允许收缩(Shrink=1)
                const bool mainAxisFixed = (isRow && wFixed) || (!isRow && hFixed);
                YGNodeStyleSetFlexShrink(node, mainAxisFixed ? 0.0F : 1.0F);
            }

            // 3. 交叉轴行为 (Cross Axis) -> AlignSelf: Stretch
            // Row布局的交叉轴是Height，Col布局的交叉轴是Width
            const bool crossAxisFill = (isRow && hFill) || (!isRow && wFill);

            if (crossAxisFill)
            {
                YGNodeStyleSetAlignSelf(node, YGAlignStretch);
            }

            // 4. 设置显式尺寸 (Width)
            if (wFixed && sizeComp->size.x() > 0)
            {
                YGNodeStyleSetWidth(node, sizeComp->size.x());
            }
            else if (wPct)
            {
                YGNodeStyleSetWidthPercent(node, sizeComp->percentage * 100.0F);
            }
            else if (wAuto)
            {
                YGNodeStyleSetWidthAuto(node);
            }

            // 5. 设置显式尺寸 (Height)
            if (hFixed && sizeComp->size.y() > 0)
            {
                YGNodeStyleSetHeight(node, sizeComp->size.y());
            }
            else if (hPct)
            {
                YGNodeStyleSetHeightPercent(node, sizeComp->percentage * 100.0F);
            }
            else if (hAuto)
            {
                // [Fix] 若 Auto 策略下已有由 RenderSystem 计算出的有效高度，则作为参考值设置
                // 这允许 "Iterative Layout"：Layout(0) -> Render(Calc Height) -> Layout(Height)
                if (sizeComp->size.y() > 0.0F)
                {
                    YGNodeStyleSetHeight(node, sizeComp->size.y());
                }
                else
                {
                    YGNodeStyleSetHeightAuto(node);
                }
            }

            // 最小/最大尺寸约束
            if (sizeComp->minSize.x() > 0) YGNodeStyleSetMinWidth(node, sizeComp->minSize.x());
            if (sizeComp->minSize.y() > 0) YGNodeStyleSetMinHeight(node, sizeComp->minSize.y());
            if (sizeComp->maxSize.x() < FLT_MAX) YGNodeStyleSetMaxWidth(node, sizeComp->maxSize.x());
            if (sizeComp->maxSize.y() < FLT_MAX) YGNodeStyleSetMaxHeight(node, sizeComp->maxSize.y());
        }

        // 5. 绝对定位处理
        if (const auto* posPolicy = Registry::TryGet<components::Position>(entity))
        {
            const bool hAbs = policies::HasFlag(posPolicy->positionPolicy, policies::Position::HAbsolute);
            const bool vAbs = policies::HasFlag(posPolicy->positionPolicy, policies::Position::VAbsolute);

            if (hAbs || vAbs)
            {
                YGNodeStyleSetPositionType(node, YGPositionTypeAbsolute);

                // 如果设置了绝对定位，并且有 Position 组件，将其值作为 Left/Top
                if (const auto* pos = Registry::TryGet<components::Position>(entity))
                {
                    if (hAbs) YGNodeStyleSetPosition(node, YGEdgeLeft, pos->value.x());
                    if (vAbs) YGNodeStyleSetPosition(node, YGEdgeTop, pos->value.y());
                }
            }
        }

        // 7. 容器子元素对齐与溢出 (LayoutInfo)
        if (const auto* layoutInfo = Registry::TryGet<components::LayoutInfo>(entity))
        {
            // 默认 FlexStart
            YGJustify justify = YGJustifyFlexStart;
            YGAlign alignItems = YGAlignFlexStart;

            const bool isRow = (layoutInfo->direction == policies::LayoutDirection::HORIZONTAL);
            const auto alignment = layoutInfo->alignment;

            auto has = [&](policies::Alignment flag) { return policies::HasFlag(alignment, flag); };

            // 主轴与交叉轴映射
            if (isRow)
            {
                // 主轴: Horizontal
                if (has(policies::Alignment::HCENTER))
                    justify = YGJustifyCenter;
                else if (has(policies::Alignment::RIGHT))
                    justify = YGJustifyFlexEnd;
                else if (has(policies::Alignment::LEFT))
                    justify = YGJustifyFlexStart;

                // 交叉轴: Vertical
                if (has(policies::Alignment::VCENTER))
                    alignItems = YGAlignCenter;
                else if (has(policies::Alignment::BOTTOM))
                    alignItems = YGAlignFlexEnd;
                else if (has(policies::Alignment::TOP))
                    alignItems = YGAlignFlexStart;
            }
            else
            {
                // 主轴: Vertical
                if (has(policies::Alignment::VCENTER))
                    justify = YGJustifyCenter;
                else if (has(policies::Alignment::BOTTOM))
                    justify = YGJustifyFlexEnd;
                else if (has(policies::Alignment::TOP))
                    justify = YGJustifyFlexStart;

                // 交叉轴: Horizontal
                if (has(policies::Alignment::HCENTER))
                    alignItems = YGAlignCenter;
                else if (has(policies::Alignment::RIGHT))
                    alignItems = YGAlignFlexEnd;
                else if (has(policies::Alignment::LEFT))
                    alignItems = YGAlignFlexStart;
            }

            YGNodeStyleSetJustifyContent(node, justify);
            YGNodeStyleSetAlignItems(node, alignItems);

            // 容器默认隐藏溢出内容，防止子元素超出边界
            // 如果是 ScrollArea 则在后面设置为 Scroll
            if (!Registry::AnyOf<components::ScrollArea>(entity))
            {
                YGNodeStyleSetOverflow(node, YGOverflowHidden);
            }
        }

        // 处理 ScrollArea
        if (Registry::AnyOf<components::ScrollArea>(entity))
        {
            YGNodeStyleSetOverflow(node, YGOverflowScroll);
        }

        // 8. 文本/按钮等叶子节点：使用 Size 组件中已有的尺寸
        if (!Registry::AnyOf<components::LayoutInfo>(entity))
        {
            // 叶子节点：如果没有设置固定尺寸，给一个合理的默认值
            if (const auto* sizeComp = Registry::TryGet<components::Size>(entity))
            {
                float defaultWidth = 100.0F;
                float defaultHeight = 20.0F;

                // 如果有文本，根据文本长度估算
                if (const auto* text = Registry::TryGet<components::Text>(entity))
                {
                    if (!text->content.empty())
                    {
                        // 粗略估算：每个字符约 8 像素宽，高度 20 像素
                        defaultWidth = static_cast<float>(text->content.length()) * 8.0F + 10.0F;
                    }
                }

                if (policies::HasFlag(sizeComp->sizePolicy, policies::Size::HAuto))
                {
                    YGNodeStyleSetMinWidth(node, defaultWidth);
                }

                if (policies::HasFlag(sizeComp->sizePolicy, policies::Size::VAuto))
                {
                    YGNodeStyleSetMinHeight(node, defaultHeight);
                }
            }
        }
    }

    /**
     * @brief 递归应用 Yoga 布局结果到 ECS 组件
     */
    static void applyYogaLayout( // NOLINT(misc-no-recursion)
        entt::entity entity,
        YGNodeRef node,
        float parentX,
        float parentY)
    {
        if (node == nullptr) return;

        // 获取 Yoga 计算的布局结果
        float left = YGNodeLayoutGetLeft(node);
        float top = YGNodeLayoutGetTop(node);
        float width = YGNodeLayoutGetWidth(node);
        float height = YGNodeLayoutGetHeight(node);

        // 安全检查：跳过无效值
        if (std::isnan(left)) left = 0.0F;
        if (std::isnan(top)) top = 0.0F;

        // 回写到 Position 组件（相对于父容器）
        if (auto* pos = Registry::TryGet<components::Position>(entity))
        {
            pos->value.x() = left;
            pos->value.y() = top;
        }

        // 回写到 Size 组件（仅当值有效时）
        if (auto* size = Registry::TryGet<components::Size>(entity))
        {
            if (!std::isnan(width) && width > 0) size->size.x() = width;
            if (!std::isnan(height) && height > 0) size->size.y() = height;
        }

        // 递归处理子节点
        const uint32_t childCount = YGNodeGetChildCount(node);
        const auto* hierarchy = Registry::TryGet<components::Hierarchy>(entity);

        float maxContentRight = 0.0F;
        float maxContentBottom = 0.0F;

        if (hierarchy != nullptr && childCount > 0)
        {
            uint32_t yogaChildIndex = 0;
            for (entt::entity child : hierarchy->children)
            {
                // Spacer 没有 Size 组件，但仍需遍历以同步 Yoga 子节点索引
                const bool isSpacer = Registry::AnyOf<components::SpacerTag>(child);
                const bool hasLayoutComponents = Registry::AllOf<components::Position, components::Size>(child);

                if (!isSpacer && !hasLayoutComponents) continue;

                if (yogaChildIndex < childCount)
                {
                    YGNodeRef childNode = YGNodeGetChild(node, yogaChildIndex);
                    applyYogaLayout(child, childNode, left, top);

                    // 收集子元素边界用于计算 ScrollArea 内容大小
                    // 注意：这里的 layout 结果是相对于父容器的
                    float childLeft = YGNodeLayoutGetLeft(childNode);
                    float childTop = YGNodeLayoutGetTop(childNode);
                    float childWidth = YGNodeLayoutGetWidth(childNode);
                    float childHeight = YGNodeLayoutGetHeight(childNode);

                    if (std::isnan(childLeft)) childLeft = 0.0F;
                    if (std::isnan(childTop)) childTop = 0.0F;
                    if (std::isnan(childWidth)) childWidth = 0.0F;
                    if (std::isnan(childHeight)) childHeight = 0.0F;

                    maxContentRight = std::max(maxContentRight, childLeft + childWidth);
                    maxContentBottom = std::max(maxContentBottom, childTop + childHeight);

                    yogaChildIndex++;
                }
            }
        }

        // 如果是 ScrollArea，更新内容大小
        if (auto* scrollArea = Registry::TryGet<components::ScrollArea>(entity))
        {
            // 获取内边距
            float paddingRight = 0.0F;
            float paddingBottom = 0.0F;
            if (auto* padding = Registry::TryGet<components::Padding>(entity))
            {
                paddingRight = padding->values.y();
                paddingBottom = padding->values.w();
            }

            scrollArea->contentSize.x() = maxContentRight + paddingRight;
            scrollArea->contentSize.y() = maxContentBottom + paddingBottom;
        }
    }

    // ===================== 窗口居中策略 =====================

    /**
     * @brief 应用窗口居中策略
     *
     * 对于顶层容器（窗口/菜单），如果没有设置固定位置，则自动居中显示。
     * 如果 Position 已有非零值，则优先使用该位置。
     */
    void applyWindowCentering(entt::entity root, float screenWidth, float screenHeight)
    {
        auto* pos = Registry::TryGet<components::Position>(root);
        auto* size = Registry::TryGet<components::Size>(root);
        if (pos == nullptr || size == nullptr) return;

        // 检查是否有 Position 组件来决定居中策略
        bool centerH = false;
        bool centerV = false;

        // 默认：如果位置是 (0,0) 且尺寸有效，则视为需要自动居中
        bool implicitCenter = (pos->value.x() == 0.0F && pos->value.y() == 0.0F);

        // 如果有明确的位置策略，检查是否要求居中
        // Position 组件中包含了策略
        if (pos != nullptr)
        {
            centerH = policies::HasFlag(pos->positionPolicy, policies::Position::HCenter);
            centerV = policies::HasFlag(pos->positionPolicy, policies::Position::VCenter);

            // 如果两个都显式指定为 FIXED，则不使用隐式居中
            if (policies::HasFlag(pos->positionPolicy, policies::Position::HFixed) &&
                policies::HasFlag(pos->positionPolicy, policies::Position::VFixed))
            {
                implicitCenter = false;
            }
        }

        if (!centerH && !centerV && implicitCenter)
        {
            centerH = true;
            centerV = true;
        }

        if (size->size.x() > 0 && size->size.y() > 0)
        {
            if (centerH) pos->value.x() = (screenWidth - size->size.x()) / 2.0F;
            if (centerV) pos->value.y() = (screenHeight - size->size.y()) / 2.0F;
        }
    }

    // ===================== 脏标记回调 =====================

    void onSizeChanged(entt::entity entity) { markEntityAndParentsDirty(entity); }

    void onLayoutInfoChanged(entt::entity entity) { Registry::EmplaceOrReplace<components::LayoutDirtyTag>(entity); }

    void onPaddingChanged(entt::entity entity) { Registry::EmplaceOrReplace<components::LayoutDirtyTag>(entity); }

    void onHierarchyChanged(entt::entity entity)
    {
        if (Registry::AnyOf<components::LayoutInfo>(entity))
        {
            Registry::EmplaceOrReplace<components::LayoutDirtyTag>(entity);
        }
    }

    void markEntityAndParentsDirty(entt::entity entity)
    {
        entt::entity current = entity;
        while (current != entt::null && Registry::Valid(current))
        {
            Registry::EmplaceOrReplace<components::LayoutDirtyTag>(current);
            const auto* hierarchy = Registry::TryGet<components::Hierarchy>(current);
            current = hierarchy != nullptr ? hierarchy->parent : entt::null;
        }
    }
};

} // namespace ui::systems