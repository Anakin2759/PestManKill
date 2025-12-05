/**
 * ************************************************************************
 *
 * @file Tabel.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief 表格组件
* 用于显示数据的表格视图组件
    一个矩形区域
    可以调整大小/设置背景颜色
    有一个横向的label
    显示表格标题（如“玩家列表”）
    有一个带滚动条的listArea 占据大部分区域
    显示多行多列的数据，每行代表一条记录，每列代表一个字段
    支持列标题显示
    支持行选中高亮显示
    支持水平和垂直滚动
    - 提供一个接口用于设置表格数据
    - 提供一个接口用于获取表格数据 返回二维数组格式
    - 提供一个接口用于清空表格数据
    - 提供一个接口用于设置表格标题
    - 提供一个接口用于获取表格标题
    - 提供一个接口用于设置对齐方式
    - 提供一个接口用于设置列宽
    - 提供一个接口用于设置行高
    - 提供一个接口用于设置滚动条
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "Widget.h"
#include <vector>
#include <string>
#include <imgui.h>

namespace ui
{

// 表格对齐方式
enum class TableAlignment : uint8_t
{
    LEFT,
    CENTER,
    RIGHT
};

/**
 * @brief 表格组件
 * 用于显示多行多列数据，支持列标题、行选中、滚动等
 */
class Tabel : public Widget
{
public:
    Tabel() = default;
    ~Tabel() override = default;
    Tabel(const Tabel&) = default;
    Tabel& operator=(const Tabel&) = default;
    Tabel(Tabel&&) = default;
    Tabel& operator=(Tabel&&) = default;

    // ===================== 数据管理 =====================
    void setData(const std::vector<std::vector<std::string>>& data)
    {
        m_data = data;
        if (m_selectedRow >= static_cast<int>(m_data.size()))
        {
            m_selectedRow = -1;
        }
    }

    [[nodiscard]] const std::vector<std::vector<std::string>>& getData() const { return m_data; }

    void clearData()
    {
        m_data.clear();
        m_selectedRow = -1;
    }

    // ===================== 标题 =====================
    void setTitle(const std::string& title) { m_title = title; }
    [[nodiscard]] const std::string& getTitle() const { return m_title; }

    // ===================== 列标题 =====================
    void setColumnHeaders(const std::vector<std::string>& headers)
    {
        m_columnHeaders = headers;
        m_columnWidths.resize(headers.size(), 0.0f); // 默认宽度为0，表示自动
        m_columnAlignments.resize(headers.size(), TableAlignment::LEFT);
    }

    // ===================== 列宽 =====================
    void setColumnWidth(size_t column, float width)
    {
        if (column < m_columnWidths.size())
        {
            m_columnWidths[column] = width;
        }
    }

    // ===================== 行高 =====================
    void setRowHeight(float height) { m_rowHeight = height; }

    // ===================== 滚动条 =====================
    void setScrollbarEnabled(bool enabled) { m_scrollbarEnabled = enabled; }

    // ===================== 对齐方式 =====================
    void setColumnAlignment(size_t column, TableAlignment alignment)
    {
        if (column < m_columnAlignments.size())
        {
            m_columnAlignments[column] = alignment;
        }
    }

    // ===================== 选中行 =====================
    [[nodiscard]] int getSelectedRow() const { return m_selectedRow; }

protected:
    void onRender(const ImVec2& position, const ImVec2& size) override
    {
        // 绘制标题
        if (!m_title.empty())
        {
            ImGui::Text("%s", m_title.c_str());
        }

        // 计算表格区域大小
        ImVec2 tableSize = size;
        if (!m_title.empty())
        {
            tableSize.y -= ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y;
        }

        // 开始子窗口用于滚动
        ImGui::BeginChild("TableScrollArea",
                          tableSize,
                          false,
                          m_scrollbarEnabled ? ImGuiWindowFlags_None : ImGuiWindowFlags_NoScrollbar);

        if (!m_data.empty() && !m_columnHeaders.empty())
        {
            // 开始表格
            ImGuiTableFlags flags =
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;
            if (ImGui::BeginTable("DataTable", static_cast<int>(m_columnHeaders.size()), flags))
            {
                // 设置列
                for (size_t i = 0; i < m_columnHeaders.size(); ++i)
                {
                    ImGuiTableColumnFlags colFlags = ImGuiTableColumnFlags_None;
                    if (m_columnWidths[i] > 0.0f)
                    {
                        colFlags |= ImGuiTableColumnFlags_WidthFixed;
                        ImGui::TableSetupColumn(m_columnHeaders[i].c_str(), colFlags, m_columnWidths[i]);
                    }
                    else
                    {
                        ImGui::TableSetupColumn(m_columnHeaders[i].c_str(), colFlags);
                    }
                }
                ImGui::TableHeadersRow();

                // 绘制数据行
                for (size_t row = 0; row < m_data.size(); ++row)
                {
                    ImGui::TableNextRow(0, m_rowHeight);

                    // 检查选中
                    bool isSelected = (static_cast<int>(row) == m_selectedRow);
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                                           isSelected ? ImGui::GetColorU32(ImGuiCol_HeaderActive) : 0);

                    for (size_t col = 0; col < m_data[row].size() && col < m_columnHeaders.size(); ++col)
                    {
                        ImGui::TableNextColumn();

                        // 对齐文本
                        ImVec2 textSize = ImGui::CalcTextSize(m_data[row][col].c_str());
                        float columnWidth = ImGui::GetContentRegionAvail().x;
                        float offsetX = 0.0f;
                        switch (m_columnAlignments[col])
                        {
                            case TableAlignment::CENTER:
                                offsetX = (columnWidth - textSize.x) * 0.5f;
                                break;
                            case TableAlignment::RIGHT:
                                offsetX = columnWidth - textSize.x;
                                break;
                            case TableAlignment::LEFT:
                            default:
                                offsetX = 0.0f;
                                break;
                        }

                        if (offsetX > 0.0f)
                        {
                            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
                        }

                        // 使整个单元格可点击以选中行
                        if (ImGui::Selectable(("##cell" + std::to_string(row) + "_" + std::to_string(col)).c_str(),
                                              isSelected,
                                              ImGuiSelectableFlags_SpanAllColumns |
                                                  ImGuiSelectableFlags_AllowItemOverlap))
                        {
                            m_selectedRow = static_cast<int>(row);
                        }
                        ImGui::SameLine();
                        ImGui::Text("%s", m_data[row][col].c_str());
                    }
                }

                ImGui::EndTable();
            }
        }

        ImGui::EndChild();
    }

private:
    std::string m_title;
    std::vector<std::string> m_columnHeaders;
    std::vector<std::vector<std::string>> m_data;
    std::vector<float> m_columnWidths;
    std::vector<TableAlignment> m_columnAlignments;
    float m_rowHeight = 20.0f;
    int m_selectedRow = -1;
    bool m_scrollbarEnabled = true;
};

} // namespace ui