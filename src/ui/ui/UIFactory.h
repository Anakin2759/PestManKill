/**
 * ************************************************************************
 *
 * @file UIFactory.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-11 (Updated)
 * @version 0.2
 * @brief UI工厂函数
 *
 * 提供创建各种UI元素的便捷方法，确保ECS实体和组件的正确初始化。
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include <string>
#include <functional>
#include "src/client/components/UIComponents.h"
#include "src/client/components/UITags.h"
#include "src/client/components/UIDefine.h"
#include "src/client/events/UIEvents.h"
#include "src/client/utils/utils.h"

namespace ui::factory
{

// 辅助宏简化注册表访问
#define REGISTRY utils::Registry::getInstance()
#define DISPATCHER utils::Dispatcher::getInstance()

/**
 * @brief 基础可渲染实体初始化 (所有可见UI元素都需要)
 */
inline entt::entity createBaseWidget()
{
    auto entity = REGISTRY.create();

    // 核心组件
    REGISTRY.emplace<components::Position>(entity);
    REGISTRY.emplace<components::Size>(entity);
    REGISTRY.emplace<components::Alpha>(entity);      // 统一替换 Visibility 结构
    REGISTRY.emplace<components::VisibleTag>(entity); // 可见性标记
    REGISTRY.emplace<components::Hierarchy>(entity);  // 允许它有父子关系

    // 原 RenderGuard 已移除，渲染系统应自行处理实体有效性

    DISPATCHER.enqueue<events::WidgetCreated>(entity);
    return entity;
}

/**
 * @brief 创建按钮
 */
inline entt::entity CreateButton(const std::string& content)
{
    auto entity = createBaseWidget();

    // 类型 Tag
    REGISTRY.emplace<components::ButtonTag>(entity);

    // 交互组件
    REGISTRY.emplace<components::Clickable>(entity);
    REGISTRY.emplace<components::ButtonState>(entity);

    // 内容组件
    auto& text = REGISTRY.emplace<components::Text>(entity);
    text.content = content;

    // 默认尺寸自适应
    REGISTRY.get<components::Size>(entity).autoSize = true;

    return entity;
}

/**
 * @brief 创建文本标签
 */
inline entt::entity CreateLabel(const std::string& content)
{
    auto entity = createBaseWidget();

    // 类型 Tag
    REGISTRY.emplace<components::LabelTag>(entity);

    // 内容组件
    auto& text = REGISTRY.emplace<components::Text>(entity);
    text.content = content;

    // 默认尺寸自适应
    REGISTRY.get<components::Size>(entity).autoSize = true;

    return entity;
}

/**
 * @brief 创建文本编辑框
 */
inline entt::entity CreateTextEdit(const std::string& placeholder = "", bool multiline = false)
{
    auto entity = createBaseWidget();

    // 类型 Tag
    REGISTRY.emplace<components::TextEditTag>(entity);

    // 内容组件
    auto& textEdit = REGISTRY.emplace<components::TextEdit>(entity);
    textEdit.placeholder = placeholder;
    textEdit.multiline = multiline;
    // buffer 默认为空

    // 文本输入框通常有固定最小尺寸
    REGISTRY.get<components::Size>(entity).minWidth = 100.0f;
    REGISTRY.get<components::Size>(entity).minHeight = multiline ? 80.0f : 30.0f;

    return entity;
}

/**
 * @brief 创建图像
 */
inline entt::entity CreateImage(void* textureId, float defaultWidth = 50.0f, float defaultHeight = 50.0f)
{
    auto entity = createBaseWidget();

    // 类型 Tag
    REGISTRY.emplace<components::ImageTag>(entity);

    // 内容组件
    auto& image = REGISTRY.emplace<components::Image>(entity);
    image.textureId = textureId;

    // 默认固定尺寸 (如果 autoSize=false)
    auto& size = REGISTRY.get<components::Size>(entity);
    size.width = defaultWidth;
    size.height = defaultHeight;
    size.autoSize = false; // 图像通常默认固定尺寸或根据父布局缩放

    return entity;
}

/**
 * @brief 创建箭头 (几何图形)
 */
inline entt::entity CreateArrow(const ImVec2& start, const ImVec2& end)
{
    auto entity = createBaseWidget();

    // 类型 Tag
    REGISTRY.emplace<components::ArrowTag>(entity);

    // 几何信息
    // ArrowTag 依赖 LineInfo (我们假设 Arrow 内部使用 LineInfo 或独立的 Arrow 组件)
    // 让我们使用 Arrow 组件代替 LineInfo：
    auto& arrow = REGISTRY.emplace<components::Arrow>(entity);
    arrow.startPoint = start;
    arrow.endPoint = end;

    // 几何图形通常由 RenderSystem 定位，Size/Position 可简化或用于包围盒
    REGISTRY.get<components::Size>(entity).autoSize = true;

    return entity;
}

/**
 * @brief 创建间隔器
 */
inline entt::entity CreateSpacer(int stretchFactor = 1)
{
    // Spacer 不需要基础组件如 VisibleTag, Alpha, Position
    auto entity = REGISTRY.create();

    REGISTRY.emplace<components::SpacerTag>(entity);
    REGISTRY.emplace<components::Hierarchy>(entity); // 允许它被添加到 Hierarchy

    auto& spacer = REGISTRY.emplace<components::Spacer>(entity);
    spacer.stretchFactor = static_cast<uint8_t>(std::max(1, stretchFactor));

    // Spacer 必须有 Size 才能让布局系统计算
    REGISTRY.emplace<components::Size>(entity);
    REGISTRY.get<components::Size>(entity).autoSize = false;
    REGISTRY.get<components::Size>(entity).width = 0.0f;
    REGISTRY.get<components::Size>(entity).height = 0.0f;

    DISPATCHER.enqueue<events::WidgetCreated>(entity);
    return entity;
}

/**
 * @brief 创建对话框
 */
#ifdef CreateDialog
#undef CreateDialog
#endif
inline entt::entity CreateDialog(std::string_view title)
{
    auto entity = createBaseWidget();

    // 类型 Tag
    REGISTRY.emplace<components::DialogTag>(entity);

    // 内容容器组件 (包含 title, min/max size)
    auto& dialog = REGISTRY.emplace<components::Window>(entity);
    dialog.title = std::string(title);
    dialog.modal = true; // 对话框通常是模态的

    // 对话框通常有固定的 Position 和 Size
    REGISTRY.get<components::Size>(entity).autoSize = false;
    REGISTRY.get<components::Size>(entity).width = 400.0f;
    REGISTRY.get<components::Size>(entity).height = 300.0f;

    // 对话框通常有一个 LayoutInfo 来排列内部元素
    REGISTRY.emplace<components::LayoutInfo>(entity);
    REGISTRY.emplace<components::Padding>(entity);

    return entity;
}

/**
 * @brief 创建窗口
 */
#ifdef CreateWindow
#undef CreateWindow
#endif
inline entt::entity CreateWindow(std::string_view title)
{
    auto entity = createBaseWidget();

    // 类型 Tag
    REGISTRY.emplace<components::WindowTag>(entity);

    // 内容容器组件
    auto& window = REGISTRY.emplace<components::Window>(entity);
    window.title = std::string(title);
    window.modal = false;

    // 窗口容器通常有固定的默认尺寸
    REGISTRY.get<components::Size>(entity).autoSize = false;
    REGISTRY.get<components::Size>(entity).width = 600.0f;
    REGISTRY.get<components::Size>(entity).height = 400.0f;

    // 窗口需要布局和内边距
    REGISTRY.emplace<components::LayoutInfo>(entity);
    REGISTRY.emplace<components::Padding>(entity);

    return entity;
}

/**
 * @brief 创建垂直布局容器 (VBox)
 */
inline entt::entity CreateVBoxLayout()
{
    auto entity = createBaseWidget();

    // 布局信息
    auto& layout = REGISTRY.emplace<components::LayoutInfo>(entity);
    layout.direction = ui::components::LayoutDirection::VERTICAL;

    // 布局容器默认自适应尺寸
    REGISTRY.get<components::Size>(entity).autoSize = true;

    // 布局容器需要内边距
    REGISTRY.emplace<components::Padding>(entity);

    return entity;
}

/**
 * @brief 创建水平布局容器 (HBox)
 */
inline entt::entity CreateHBoxLayout()
{
    auto entity = createBaseWidget();

    // 布局信息
    auto& layout = REGISTRY.emplace<components::LayoutInfo>(entity);
    layout.direction = ui::components::LayoutDirection::HORIZONTAL;

    // 布局容器默认自适应尺寸
    REGISTRY.get<components::Size>(entity).autoSize = true;

    // 布局容器需要内边距
    REGISTRY.emplace<components::Padding>(entity);

    return entity;
}

#undef REGISTRY
#undef DISPATCHER

} // namespace ui::factory