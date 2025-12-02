/**
 * ************************************************************************
 *
 * @file Spacer.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-02
 * @version 0.1
 * @brief
 * 空白占位组件定义
 * 模拟 Qt 的空白占位组件 QSpacerItem
 * 支持水平和垂直方向的空白占位
 * 支持设置固定大小或可伸缩大小
 * 基于ImGui实现渲染
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "Widget.h"
#include <imgui.h>

namespace ui
{
/**
 * @brief 空白占位组件，用于在布局中创建空白间隔
 */
class Spacer : public Widget
{
public:
    /**
     * @brief 空白占位策略
     */
    enum class Policy : uint8_t
    {
        FIXED,    // 固定大小
        MINIMUM,  // 最小大小，可扩展
        EXPANDING // 可扩展，占用所有可用空间
    };

    /**
     * @brief 空白占位方向
     */
    enum class Orientation : uint8_t
    {
        HORIZONTAL, // 水平方向
        VERTICAL    // 垂直方向
    };

    /**
     * @brief 构造固定大小的空白占位
     * @param width 宽度（水平方向时有效）
     * @param height 高度（垂直方向时有效）
     */
    explicit Spacer(float width = 0.0F, float height = 0.0F)
        : m_orientation(Orientation::HORIZONTAL), m_policy(Policy::FIXED), m_minSize{width, height}
    {
        if (width > 0.0F || height > 0.0F)
        {
            setFixedSize(width, height);
        }
    }

    /**
     * @brief 构造可伸缩的空白占位
     * @param orientation 方向
     * @param policy 伸缩策略
     * @param minSize 最小尺寸
     */
    explicit Spacer(Orientation orientation, Policy policy = Policy::EXPANDING, float minSize = 0.0F)
        : m_orientation(orientation), m_policy(policy)
    {
        if (orientation == Orientation::HORIZONTAL)
        {
            m_minSize = {minSize, 0.0F};
        }
        else
        {
            m_minSize = {0.0F, minSize};
        }
    }

    ~Spacer() override = default;

    Spacer(const Spacer&) = delete;
    Spacer& operator=(const Spacer&) = delete;
    Spacer(Spacer&&) = delete;
    Spacer& operator=(Spacer&&) = delete;

    /**
     * @brief 设置空白占位策略
     * @param policy 策略
     */
    void setPolicy(Policy policy) { m_policy = policy; }

    /**
     * @brief 设置方向
     * @param orientation 方向
     */
    void setOrientation(Orientation orientation) { m_orientation = orientation; }

    /**
     * @brief 设置最小尺寸
     * @param width 最小宽度
     * @param height 最小高度
     */
    void setMinimumSize(float width, float height) { m_minSize = {width, height}; }

    /**
     * @brief 获取策略
     * @return 伸缩策略
     */
    [[nodiscard]] Policy getPolicy() const { return m_policy; }

    /**
     * @brief 获取方向
     * @return 方向
     */
    [[nodiscard]] Orientation getOrientation() const { return m_orientation; }

    /**
     * @brief 是否为可扩展的
     * @return true表示可扩展
     */
    [[nodiscard]] bool isExpanding() const { return m_policy == Policy::EXPANDING; }

    /**
     * @brief 计算占位大小
     * @return 尺寸
     */
    ImVec2 calculateSize() override
    {
        if (isFixedSize())
        {
            // 使用基类的方法获取固定尺寸
            return Widget::calculateSize();
        }
        return m_minSize;
    }

protected:
    /**
     * @brief 渲染空白占位（实际不渲染任何内容）
     * @param position 位置
     * @param size 尺寸
     */
    void onRender(const ImVec2& position, const ImVec2& size) override
    {
// Spacer 不渲染任何内容，仅占用空间
// 可选：调试模式下绘制边框
#ifdef SPACER_DEBUG
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRect(
            position, ImVec2(position.x + size.x, position.y + size.y), IM_COL32(255, 0, 0, 100), 0.0F, 0, 1.0F);
#endif
    }

private:
    Orientation m_orientation;    // 方向
    Policy m_policy;              // 伸缩策略
    ImVec2 m_minSize{0.0F, 0.0F}; // 最小尺寸
};

/**
 * @brief 创建水平空白占位的便捷函数
 * @param width 宽度，0表示可扩展
 * @return Spacer智能指针
 */
inline std::shared_ptr<Spacer> CreateHorizontalSpacer(float width = 0.0F)
{
    if (width > 0.0F)
    {
        return std::make_shared<Spacer>(width, 0.0F);
    }
    return std::make_shared<Spacer>(Spacer::Orientation::HORIZONTAL, Spacer::Policy::EXPANDING);
}

/**
 * @brief 创建垂直空白占位的便捷函数
 * @param height 高度，0表示可扩展
 * @return Spacer智能指针
 */
inline std::shared_ptr<Spacer> CreateVerticalSpacer(float height = 0.0F)
{
    if (height > 0.0F)
    {
        return std::make_shared<Spacer>(0.0F, height);
    }
    return std::make_shared<Spacer>(Spacer::Orientation::VERTICAL, Spacer::Policy::EXPANDING);
}

/**
 * @brief 创建固定尺寸空白占位的便捷函数
 * @param width 宽度
 * @param height 高度
 * @return Spacer智能指针
 */
inline std::shared_ptr<Spacer> CreateFixedSpacer(float width, float height)
{
    return std::make_shared<Spacer>(width, height);
}

} // namespace ui
