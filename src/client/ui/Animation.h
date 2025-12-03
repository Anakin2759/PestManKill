/**
 * ************************************************************************
 *
 * @file AAnime.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-01
 * @version 0.1
 * @brief 动画组件定义
  模拟 Qt 的动画组件 QPropertyAnimation
  用于实现简单的属性动画效果
  支持位置、大小、透明度等属性的动画
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "Widget.h"
#include <cstdint>
#include <functional>
#include <cmath>
namespace ui
{
enum class AnimationProperty : uint8_t
{
    POSITION,
    SIZE,
    OPACITY
};

enum class EasingType : uint8_t
{
    LINEAR,           // 线性
    EASE_IN_QUAD,     // 二次方缓入
    EASE_OUT_QUAD,    // 二次方缓出
    EASE_IN_OUT_QUAD, // 二次方缓入缓出
    EASE_IN_CUBIC,    // 三次方缓入
    EASE_OUT_CUBIC,   // 三次方缓出
    EASE_IN_OUT_CUBIC // 三次方缓入缓出
};

class Animation
{
public:
    using FinishedCallback = std::function<void()>;
    using UpdateCallback = std::function<void(float /*progress*/)>;

    Animation(Widget* target,
              AnimationProperty property,
              float startValue1,
              float startValue2,
              float endValue1,
              float endValue2,
              uint32_t durationMs)
        : m_target(target), m_property(property), m_startValue1(startValue1), m_startValue2(startValue2),
          m_endValue1(endValue1), m_endValue2(endValue2), m_durationMs(durationMs)
    {
    }

    // ===================== 控制方法 =====================
    void start()
    {
        m_running = true;
        m_paused = false;
        m_elapsedMs = 0;
    }

    void stop()
    {
        m_running = false;
        m_paused = false;
        m_elapsedMs = 0;
    }

    void pause()
    {
        if (m_running)
        {
            m_paused = true;
        }
    }

    void resume()
    {
        if (m_running)
        {
            m_paused = false;
        }
    }

    void restart()
    {
        m_elapsedMs = 0;
        m_running = true;
        m_paused = false;
    }

    // ===================== 更新方法 =====================
    void update(uint32_t deltaMs)
    {
        if (!m_running || m_paused)
        {
            return;
        }

        m_elapsedMs += deltaMs;
        float rawProgress = static_cast<float>(m_elapsedMs) / static_cast<float>(m_durationMs);

        if (rawProgress >= 1.0F)
        {
            rawProgress = 1.0F;

            // 应用最终值
            applyValues(1.0F);

            // 处理循环
            if (m_loopCount != 0)
            {
                m_elapsedMs = 0;
                if (m_loopCount > 0)
                {
                    --m_loopCount;
                }
                if (m_reverse)
                {
                    std::swap(m_startValue1, m_endValue1);
                    std::swap(m_startValue2, m_endValue2);
                }
            }
            else
            {
                m_running = false;
                if (m_finishedCallback)
                {
                    m_finishedCallback();
                }
            }
        }
        else
        {
            // 应用缓动函数
            float easedProgress = applyEasing(rawProgress);
            applyValues(easedProgress);

            if (m_updateCallback)
            {
                m_updateCallback(easedProgress);
            }
        }
    }

    // ===================== 配置方法 =====================
    void setEasingType(EasingType easing) { m_easingType = easing; }

    void setLoopCount(int32_t count)
    {
        m_loopCount = count; // -1 表示无限循环
    }

    void setReverse(bool reverse) { m_reverse = reverse; }

    void setDuration(uint32_t durationMs) { m_durationMs = durationMs; }

    void setFinishedCallback(FinishedCallback callback) { m_finishedCallback = std::move(callback); }

    void setUpdateCallback(UpdateCallback callback) { m_updateCallback = std::move(callback); }

    // ===================== 查询方法 =====================
    [[nodiscard]] bool isRunning() const { return m_running; }
    [[nodiscard]] bool isPaused() const { return m_paused; }
    [[nodiscard]] float getProgress() const
    {
        if (m_durationMs == 0)
        {
            return 0.0F;
        }
        return std::min(1.0F, static_cast<float>(m_elapsedMs) / static_cast<float>(m_durationMs));
    }
    [[nodiscard]] uint32_t getElapsedMs() const { return m_elapsedMs; }
    [[nodiscard]] uint32_t getDurationMs() const { return m_durationMs; }

private:
    // 应用缓动函数
    [[nodiscard]] float applyEasing(float linearProgress) const
    {
        constexpr float HALF = 0.5F;
        constexpr float TWO = 2.0F;
        constexpr float THREE = 3.0F;
        constexpr float FOUR = 4.0F;

        switch (m_easingType)
        {
            case EasingType::LINEAR:
                return linearProgress;

            case EasingType::EASE_IN_QUAD:
                return linearProgress * linearProgress;

            case EasingType::EASE_OUT_QUAD:
                return linearProgress * (TWO - linearProgress);

            case EasingType::EASE_IN_OUT_QUAD:
                if (linearProgress < HALF)
                {
                    return TWO * linearProgress * linearProgress;
                }
                return 1.0F - (std::pow((-TWO * linearProgress) + TWO, TWO) / TWO);

            case EasingType::EASE_IN_CUBIC:
                return linearProgress * linearProgress * linearProgress;

            case EasingType::EASE_OUT_CUBIC:
                return 1.0F - std::pow(1.0F - linearProgress, THREE);

            case EasingType::EASE_IN_OUT_CUBIC:
                if (linearProgress < HALF)
                {
                    return FOUR * linearProgress * linearProgress * linearProgress;
                }
                return 1.0F - (std::pow((-TWO * linearProgress) + TWO, THREE) / TWO);

            default:
                return linearProgress;
        }
    }

    // 应用当前值到目标 widget
    void applyValues(float progress)
    {
        float currentValue1 = m_startValue1 + ((m_endValue1 - m_startValue1) * progress);
        float currentValue2 = m_startValue2 + ((m_endValue2 - m_startValue2) * progress);

        switch (m_property)
        {
            case AnimationProperty::POSITION:
                m_target->setPosition(currentValue1, currentValue2);
                break;

            case AnimationProperty::SIZE:
                m_target->setFixedSize(currentValue1, currentValue2);
                break;

            case AnimationProperty::OPACITY:
                m_target->setAlpha(currentValue1);
                break;
        }
    }

    // ===================== 成员变量 =====================
    Widget* m_target;
    AnimationProperty m_property;
    EasingType m_easingType = EasingType::LINEAR;

    float m_startValue1;
    float m_startValue2;
    float m_endValue1;
    float m_endValue2;

    uint32_t m_durationMs;
    uint32_t m_elapsedMs = 0;

    int32_t m_loopCount = 0; // 0=不循环, -1=无限循环, >0=循环次数
    bool m_reverse = false;  // 循环时是否反向播放
    bool m_running = false;
    bool m_paused = false;

    FinishedCallback m_finishedCallback;
    UpdateCallback m_updateCallback;
};
} // namespace ui