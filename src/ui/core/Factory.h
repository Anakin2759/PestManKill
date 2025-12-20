/**
 * ************************************************************************
 *
 * @file Factory.h
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
#include <string_view>
#include <functional>
#include "src/ui/components/Components.h"
#include "src/ui/components/Tags.h"
#include "src/ui/components/Define.h"
#include "src/ui/components/Events.h"
#include <utils.h>
namespace ui::factory
{

/**
 * @brief 基础可渲染实体初始化 (所有可见UI元素都需要)
 */
inline entt::entity createBaseWidget()
{
    auto entity = utils::Registry::getInstance().create();

    // 核心组件
    utils::Registry::getInstance().emplace<components::Position>(entity);
    utils::Registry::getInstance().emplace<components::Size>(entity);
    utils::Registry::getInstance().emplace<components::Alpha>(entity);      // 统一替换 Visibility 结构
    utils::Registry::getInstance().emplace<components::VisibleTag>(entity); // 可见性标记
    utils::Registry::getInstance().emplace<components::Hierarchy>(entity);  // 允许它有父子关系

    // 原 RenderGuard 已移除，渲染系统应自行处理实体有效性

    return entity;
}

// =======================================================
// Compatibility helpers (Scene/UI 便捷接口)
// =======================================================

inline void CreateFadeInAnimation(entt::entity entity, float duration)
{
    if (!utils::Registry::getInstance().valid(entity)) return;

    // 确保有 Alpha
    auto& alpha = utils::Registry::getInstance().get_or_emplace<components::Alpha>(entity);
    alpha.value = 0.0f;

    auto& time = utils::Registry::getInstance().get_or_emplace<components::AnimationTime>(entity);
    time.duration = duration;
    time.elapsed = 0.0f;

    auto& alphaAnim = utils::Registry::getInstance().get_or_emplace<components::AnimationAlpha>(entity);
    alphaAnim.from = 0.0f;
    alphaAnim.to = 1.0f;

    utils::Registry::getInstance().emplace_or_replace<components::AnimatingTag>(entity);
}

/**
 * @brief 创建按钮
 */
inline entt::entity CreateButton(const std::string& content)
{
    auto entity = createBaseWidget();

    // 类型 Tag
    utils::Registry::getInstance().emplace<components::ButtonTag>(entity);

    // 交互组件
    utils::Registry::getInstance().emplace<components::Clickable>(entity);
    utils::Registry::getInstance().emplace<components::ButtonState>(entity);

    // 内容组件
    auto& text = utils::Registry::getInstance().emplace<components::Text>(entity);
    text.content = content;

    // 默认尺寸自适应
    utils::Registry::getInstance().get<components::Size>(entity).autoSize = true;

    return entity;
}

/**
 * @brief 创建文本标签
 */
inline entt::entity CreateLabel(const std::string& content)
{
    auto entity = createBaseWidget();

    // 类型 Tag
    utils::Registry::getInstance().emplace<components::LabelTag>(entity);

    // 内容组件
    auto& text = utils::Registry::getInstance().emplace<components::Text>(entity);
    text.content = content;

    // 默认尺寸自适应
    utils::Registry::getInstance().get<components::Size>(entity).autoSize = true;

    return entity;
}

/**
 * @brief 创建文本编辑框
 */
inline entt::entity CreateTextEdit(const std::string& placeholder = "", bool multiline = false)
{
    auto entity = createBaseWidget();

    // 类型 Tag
    utils::Registry::getInstance().emplace<components::TextEditTag>(entity);

    // 内容组件
    auto& textEdit = utils::Registry::getInstance().emplace<components::TextEdit>(entity);
    textEdit.placeholder = placeholder;
    textEdit.multiline = multiline;
    // buffer 默认为空

    // 文本输入框通常有固定最小尺寸
    utils::Registry::getInstance().get<components::Size>(entity).minSize = {100.0f, multiline ? 80.0f : 30.0f};

    return entity;
}

/**
 * @brief 创建图像
 */
inline entt::entity CreateImage(void* textureId, float defaultWidth = 50.0f, float defaultHeight = 50.0f)
{
    auto entity = createBaseWidget();

    // 类型 Tag
    utils::Registry::getInstance().emplace<components::ImageTag>(entity);

    // 内容组件
    auto& image = utils::Registry::getInstance().emplace<components::Image>(entity);
    image.textureId = textureId;

    // 默认固定尺寸 (如果 autoSize=false)
    auto& size = utils::Registry::getInstance().get<components::Size>(entity);
    size.size = {defaultWidth, defaultHeight};
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
    utils::Registry::getInstance().emplace<components::ArrowTag>(entity);

    // 几何信息
    // ArrowTag 依赖 LineInfo (我们假设 Arrow 内部使用 LineInfo 或独立的 Arrow 组件)
    // 让我们使用 Arrow 组件代替 LineInfo：
    auto& arrow = utils::Registry::getInstance().emplace<components::Arrow>(entity);
    arrow.startPoint = start;
    arrow.endPoint = end;

    // 几何图形通常由 RenderSystem 定位，Size/Position 可简化或用于包围盒
    utils::Registry::getInstance().get<components::Size>(entity).autoSize = true;

    return entity;
}

/**
 * @brief 创建间隔器
 */
inline entt::entity CreateSpacer(int stretchFactor = 1)
{
    // Spacer 不需要基础组件如 VisibleTag, Alpha, Position
    auto entity = utils::Registry::getInstance().create();

    utils::Registry::getInstance().emplace<components::SpacerTag>(entity);
    utils::Registry::getInstance().emplace<components::Hierarchy>(entity); // 允许它被添加到 Hierarchy

    // 参与布局必须具备 Position/Size
    utils::Registry::getInstance().emplace<components::Position>(entity);

    auto& spacer = utils::Registry::getInstance().emplace<components::Spacer>(entity);
    spacer.stretchFactor = static_cast<uint8_t>(std::max(1, stretchFactor));

    // Spacer 必须有 Size 才能让布局系统计算
    utils::Registry::getInstance().emplace<components::Size>(entity);
    utils::Registry::getInstance().get<components::Size>(entity).autoSize = false;
    utils::Registry::getInstance().get<components::Size>(entity).size = {0.0f, 0.0f};

    return entity;
}

/**
 * @brief 创建固定尺寸的“空白占位”元素（用于固定间距；不使用 SpacerTag，避免被当作拉伸占位）
 */
inline entt::entity CreateSpacer(float width, float height)
{
    auto entity = createBaseWidget();
    auto& size = utils::Registry::getInstance().get<components::Size>(entity);
    size.size = {width, height};
    size.autoSize = false;
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
    utils::Registry::getInstance().emplace<components::DialogTag>(entity);

    // 内容容器组件 (包含 title, min/max size)
    auto& dialog = utils::Registry::getInstance().emplace<components::Window>(entity);
    dialog.title = std::string(title);
    dialog.modal = true; // 对话框通常是模态的

    // 对话框通常有固定的 Position 和 Size
    utils::Registry::getInstance().get<components::Size>(entity).autoSize = false;
    utils::Registry::getInstance().get<components::Size>(entity).size = {400.0f, 300.0f};

    // 对话框通常有一个 LayoutInfo 来排列内部元素
    utils::Registry::getInstance().emplace<components::LayoutInfo>(entity);
    utils::Registry::getInstance().emplace<components::Padding>(entity);

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
    utils::Registry::getInstance().emplace<components::WindowTag>(entity);

    // 内容容器组件
    auto& window = utils::Registry::getInstance().emplace<components::Window>(entity);
    window.title = std::string(title);
    window.modal = false;

    // 窗口容器通常有固定的默认尺寸
    utils::Registry::getInstance().get<components::Size>(entity).autoSize = false;
    utils::Registry::getInstance().get<components::Size>(entity).size = {600.0f, 400.0f};

    // 窗口需要布局和内边距
    utils::Registry::getInstance().emplace<components::LayoutInfo>(entity);
    utils::Registry::getInstance().emplace<components::Padding>(entity);

    return entity;
}

/**
 * @brief 创建垂直布局容器 (VBox)
 */
inline entt::entity CreateVBoxLayout()
{
    auto entity = createBaseWidget();

    // 布局信息
    auto& layout = utils::Registry::getInstance().emplace<components::LayoutInfo>(entity);
    layout.direction = ui::components::LayoutDirection::VERTICAL;

    // 布局容器默认自适应尺寸
    utils::Registry::getInstance().get<components::Size>(entity).autoSize = true;

    // 布局容器需要内边距
    utils::Registry::getInstance().emplace<components::Padding>(entity);

    return entity;
}

/**
 * @brief 创建水平布局容器 (HBox)
 */
inline entt::entity CreateHBoxLayout()
{
    auto entity = createBaseWidget();

    // 布局信息
    auto& layout = utils::Registry::getInstance().emplace<components::LayoutInfo>(entity);
    layout.direction = ui::components::LayoutDirection::HORIZONTAL;

    // 布局容器默认自适应尺寸
    utils::Registry::getInstance().get<components::Size>(entity).autoSize = true;

    // 布局容器需要内边距
    utils::Registry::getInstance().emplace<components::Padding>(entity);

    return entity;
}

inline entt::entity CreateTextInput(std::string_view initialText = "", std::string_view placeholder = "")
{
    auto entity = CreateTextEdit(std::string(placeholder), false);
    auto& edit = utils::Registry::getInstance().get<components::TextEdit>(entity);
    edit.buffer = std::string(initialText);
    return entity;
}

inline entt::entity CreateTextArea(std::string_view initialText = "", std::string_view placeholder = "")
{
    auto entity = CreateTextEdit(std::string(placeholder), true);
    auto& edit = utils::Registry::getInstance().get<components::TextEdit>(entity);
    edit.buffer = std::string(initialText);
    return entity;
}

/**
 * @brief 创建主窗口实体
 */
inline entt::entity createMainWindow()
{
    auto entity = CreateWindow("Main Window");

    return entity;
}

} // namespace ui::factory