/**
 * ************************************************************************
 *
 * @file Arrow.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-02
 * @version 0.2
 * @brief 箭头特效组件定义
    从一个角色的image组件指向另一个角色的image组件 起点终点取二者最小距离线段
    可能实现是一个背景透明的image组件
    通过计算起点终点位置和角度 实现箭头的旋转
    起点位置是image顶部中心点 终点位置是image底部中心点

    - 提供设置箭头Image的方法 setArrowImage
    - 提供设置起点和终点的方法 setStartEnd
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "Widget.h"
#include <imgui.h>
#include <cmath>

namespace ui
{

/**
 * @brief 箭头组件，用于绘制从一个点到另一个点的箭头
 */
class Arrow : public Widget
{
public:
    Arrow() = default;
    ~Arrow() override = default;

    Arrow(const Arrow&) = delete;
    Arrow& operator=(const Arrow&) = delete;
    Arrow(Arrow&&) = delete;
    Arrow& operator=(Arrow&&) = delete;

    // ===================== 位置设置 =====================

    /**
     * @brief 设置箭头的起点和终点
     * @param start 起点坐标
     * @param end 终点坐标
     */
    void setStartEnd(const ImVec2& start, const ImVec2& end)
    {
        m_start = start;
        m_end = end;
        m_isDirty = true;
    }

    /**
     * @brief 设置起点
     */
    void setStart(const ImVec2& start)
    {
        m_start = start;
        m_isDirty = true;
    }

    /**
     * @brief 设置终点
     */
    void setEnd(const ImVec2& end)
    {
        m_end = end;
        m_isDirty = true;
    }

    // ===================== 样式设置 =====================

    /**
     * @brief 设置箭头颜色
     */
    void setColor(const ImVec4& color) { m_color = color; }

    /**
     * @brief 设置箭头线宽
     */
    void setThickness(float thickness) { m_thickness = std::max(1.0F, thickness); }

    /**
     * @brief 设置箭头头部大小
     */
    void setArrowHeadSize(float size) { m_arrowHeadSize = std::max(1.0F, size); }

    /**
     * @brief 设置是否绘制箭头头部
     */
    void setDrawArrowHead(bool draw) { m_drawArrowHead = draw; }

    // ===================== 获取器 =====================

    [[nodiscard]] ImVec2 getStart() const { return m_start; }
    [[nodiscard]] ImVec2 getEnd() const { return m_end; }
    [[nodiscard]] ImVec4 getColor() const { return m_color; }
    [[nodiscard]] float getThickness() const { return m_thickness; }

protected:
    void onRender(const ImVec2& /*position*/, const ImVec2& /*size*/) override
    {
        if (m_isDirty)
        {
            calculateArrow();
            m_isDirty = false;
        }

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImU32 color = ImGui::GetColorU32(m_color);

        // 绘制箭头线段
        drawList->AddLine(m_start, m_end, color, m_thickness);

        // 绘制箭头头部
        if (m_drawArrowHead)
        {
            drawArrowHead(drawList, color);
        }
    }

private:
    static constexpr float PI = 3.14159265358979323846F;
    static constexpr float ARROW_ANGLE = PI / 6.0F; // 30度

    void calculateArrow()
    {
        // 计算箭头方向向量
        float dx = m_end.x - m_start.x;
        float dy = m_end.y - m_start.y;
        float length = std::sqrt(dx * dx + dy * dy);

        if (length < 0.001F)
        {
            // 起点终点重合
            return;
        }

        // 归一化方向向量
        m_directionX = dx / length;
        m_directionY = dy / length;
    }

    void drawArrowHead(ImDrawList* drawList, ImU32 color)
    {
        // 计算箭头头部的两个点
        float angle1 = std::atan2(m_directionY, m_directionX) + PI - ARROW_ANGLE;
        float angle2 = std::atan2(m_directionY, m_directionX) + PI + ARROW_ANGLE;

        ImVec2 p1(m_end.x + m_arrowHeadSize * std::cos(angle1), m_end.y + m_arrowHeadSize * std::sin(angle1));
        ImVec2 p2(m_end.x + m_arrowHeadSize * std::cos(angle2), m_end.y + m_arrowHeadSize * std::sin(angle2));

        // 绘制箭头头部（三角形）
        drawList->AddTriangleFilled(m_end, p1, p2, color);
    }

    ImVec2 m_start{0.0F, 0.0F};
    ImVec2 m_end{0.0F, 0.0F};
    ImVec4 m_color{1.0F, 1.0F, 1.0F, 1.0F};

    float m_thickness = 2.0F;
    float m_arrowHeadSize = 10.0F;
    float m_directionX = 0.0F;
    float m_directionY = 0.0F;

    bool m_drawArrowHead = true;
    bool m_isDirty = true;
};

} // namespace ui
