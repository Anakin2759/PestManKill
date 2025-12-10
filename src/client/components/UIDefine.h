#pragma once
namespace ui::components
{
/**
 * @brief 布局方向枚举
 */
enum class LayoutDirection : uint8_t
{
    HORIZONTAL,
    VERTICAL
};

/**
 * @brief 对齐方式枚举
 */
enum class Alignment : uint8_t
{
    NONE = 0,
    LEFT = 1 << 0,
    RIGHT = 1 << 1,
    TOP = 1 << 2,
    BOTTOM = 1 << 3,
    HCENTER = 1 << 4,
    VCENTER = 1 << 5,
};
/**
 * @brief 动画播放模式枚举
 */
enum class PlayMode : uint8_t
{
    ONCE,    // 单次播放
    LOOP,    // 循环
    PINGPONG // 往返
};
/**
 * @brief 动画缓动类型枚举
 */
enum class EasingType : uint8_t
{
    Linear,    // 线性
    EaseIn,    // 缓入
    EaseOut,   // 缓出
    EaseInOut, // 缓入缓出
    Custom     // 自定义
};
/**
 * @brief 交互类型枚举
 */
enum class InteractionType : uint8_t
{
    None,
    Click,
    UseCard,
    SelectTarget,
    Confirm,
    Cancel
};

/**
 * @brief 按钮状态组件
 */
enum class ButtonVisualState : uint8_t
{
    Idle,    // 默认状态
    Hover,   // 鼠标悬停
    Pressed, // 鼠标按下
    Disabled // 不可用状态
};
} // namespace ui::components
