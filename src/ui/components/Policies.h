/**
 * ************************************************************************
 *
 * @file Policies.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @brief UI 相关的全局定义 (优化后)
 *
 * ************************************************************************
 */
#pragma once
#include <cstdint>

namespace ui::policies
{
/**
 * @brief 布局方向枚举
 */
enum class LayoutDirection : uint8_t
{
    HORIZONTAL, // 0
    VERTICAL    // 1
};

/**
 * @brief 对齐方式枚举 (使用完整的位标记实现多重对齐)
 * 例如：CENTER = HCENTER | VCENTER
 */
enum class Alignment : uint8_t
{
    NONE = 0,

    // 水平对齐 (Horizontal Flags)
    LEFT = 1 << 0,    // 0x01
    HCENTER = 1 << 1, // 0x02
    RIGHT = 1 << 2,   // 0x04

    // 垂直对齐 (Vertical Flags)
    TOP = 1 << 3,     // 0x08
    VCENTER = 1 << 4, // 0x10
    BOTTOM = 1 << 5,  // 0x20

    // 组合常用对齐方式 (可选，但非常实用)
    CENTER = HCENTER | VCENTER, // 0x12
    TOP_LEFT = TOP | LEFT,      // 0x09
};

/**
 * @brief 动画播放模式枚举
 */
enum class Play : uint8_t
{
    ONCE,    // 单次播放
    LOOP,    // 循环
    PINGPONG // 往返
};

/**
 * @brief 动画缓动类型枚举 (使用专业名称)
 */
enum class Easing : uint8_t
{
    Linear, // 线性

    // 常用缓动曲线
    EaseInSine,    // 正弦缓入
    EaseOutSine,   // 正弦缓出
    EaseInOutSine, // 正弦缓入缓出

    EaseInQuad,    // 二次方缓入
    EaseOutQuad,   // 二次方缓出
    EaseInOutQuad, // 二次方缓入缓出

    Custom // 自定义 (例如通过函数指针 Component)
};

/**
 * @brief 交互类型枚举 (注意：建议通过 ECS Event 触发动作，此枚举用于标记UI意图)
 */
enum class Interaction : uint8_t
{
    None,
    Click,
    Submit,    // 确认/提交动作 (取代 Confirm)
    Cancel,    // 取消
    DragStart, // 拖拽开始
    DragEnd,   // 拖拽结束
    // 特定游戏逻辑如 UseCard, SelectTarget 建议作为 Component 上的数据或 Event 载荷
};

/**
 * @brief 按钮视觉状态枚举
 */
enum class ButtonVisual : uint8_t
{
    Idle,    // 默认状态
    Hover,   // 鼠标悬停
    Pressed, // 鼠标按下 (通常等于 Active 状态)
    Disabled // 不可用状态
};

enum class Focus : uint8_t
{
    NoFocus,    // 不接受焦点
    TabFocus,   // 通过 Tab 键接受焦点
    ClickFocus, // 通过鼠标点击接受焦点
    StrongFocus // 通过 Tab 键和鼠标点击均可接受焦点
};

/**
 * @brief 尺寸策略枚举
 */
enum class Size : uint8_t
{
    Fixed,     // 固定尺寸
    Auto,      // 根据内容自动调整
    FillParent // 填满父容器 (减去内边距)
};

} // namespace ui::policies