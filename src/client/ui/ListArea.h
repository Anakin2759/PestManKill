/**
 * ************************************************************************
 *
 * @file ListArea.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-02
 * @version 0.2
 * @brief
 * 列表区域组件定义
    基于ImGui实现列表渲染
    支持添加、删除widget，管理子组件
    支持批量添加widget
    支持水平和垂直方向排列

    支持单选和多选模式
    支持设置项高度和间距

    可以横向或纵向排列
    必要时组件才重叠
    可以设置对齐方式 左（上）/居中/右（下）
    可以设置滚动条/无滚动条
    可以设置组件间距
    可以设置边距
    不单独处理文本/图片，由外部widget决定
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "Widget.h"
#include <vector>
#include <imgui.h>
#include "src/client/utils/Logger.h"
namespace ui
{

// 列表方向
enum class ListDirection : uint8_t
{
    HORIZONTAL, // 横向排列
    VERTICAL    // 纵向排列
};

// 对齐方式
enum class ListAlignment : uint8_t
{
    START,  // 左（横向）或上（纵向）
    CENTER, // 居中
    END     // 右（横向）或下（纵向）
};

// 选择模式
enum class SelectionMode : uint8_t
{
    NONE,    // 不可选择
    SINGLE,  // 单选
    MULTIPLE // 多选
};

// 列表项结构
struct ListItem
{
    Widget::Ptr widget;
    bool selected = false;
    float customHeight = 0.0F; // 0 表示使用默认高度
};

/**
 * @brief 列表区域组件
 * 支持横向或纵向排列widget
 * 支持单选/多选模式
 * 支持滚动条
 * 支持设置对齐方式、间距、边距
 */
class ListArea : public Widget
{
public:
    ListArea() = default;
    explicit ListArea(ListDirection direction) : m_direction(direction) {}
    ~ListArea() override = default;
    ListArea(const ListArea&) = default;
    ListArea& operator=(const ListArea&) = default;
    ListArea(ListArea&&) = default;
    ListArea& operator=(ListArea&&) = default;

    // ===================== 添加和删除 Widget =====================
    void addWidget(Widget::Ptr widget, float customHeight = 0.0F)
    {
        if (!widget)
        {
            return;
        }
        m_items.push_back({std::move(widget), false, customHeight});
        addChild(m_items.back().widget);
    }

    void addWidgets(const std::vector<Widget::Ptr>& widgets)
    {
        for (const auto& widget : widgets)
        {
            addWidget(widget);
        }
    }

    void removeWidget(const Widget::Ptr& widget)
    {
        m_items.erase(std::remove_if(m_items.begin(),
                                     m_items.end(),
                                     [&widget](const ListItem& item) { return item.widget == widget; }),
                      m_items.end());
    }

    void clear() { m_items.clear(); }

    [[nodiscard]] size_t getItemCount() const { return m_items.size(); }

    // ===================== 选择管理 =====================
    void setSelectionMode(SelectionMode mode) { m_selectionMode = mode; }
    [[nodiscard]] SelectionMode getSelectionMode() const { return m_selectionMode; }

    void selectItem(size_t index)
    {
        if (index >= m_items.size())
        {
            return;
        }

        if (m_selectionMode == SelectionMode::NONE)
        {
            return;
        }

        if (m_selectionMode == SelectionMode::SINGLE)
        {
            // 单选模式：清除其他选择
            for (auto& item : m_items)
            {
                item.selected = false;
            }
        }

        m_items[index].selected = true;
    }

    void deselectItem(size_t index)
    {
        if (index >= m_items.size())
        {
            return;
        }
        m_items[index].selected = false;
    }

    void clearSelection()
    {
        for (auto& item : m_items)
        {
            item.selected = false;
        }
    }

    [[nodiscard]] bool isItemSelected(size_t index) const
    {
        if (index >= m_items.size())
        {
            return false;
        }
        return m_items[index].selected;
    }

    [[nodiscard]] std::vector<size_t> getSelectedIndices() const
    {
        std::vector<size_t> indices;
        for (size_t i = 0; i < m_items.size(); ++i)
        {
            if (m_items[i].selected)
            {
                indices.push_back(i);
            }
        }
        return indices;
    }

    // ===================== 布局配置 =====================
    void setDirection(ListDirection direction) { m_direction = direction; }
    [[nodiscard]] ListDirection getDirection() const { return m_direction; }

    void setAlignment(ListAlignment alignment) { m_alignment = alignment; }
    [[nodiscard]] ListAlignment getAlignment() const { return m_alignment; }

    void setItemSpacing(float spacing) { m_itemSpacing = std::max(0.0F, spacing); }
    [[nodiscard]] float getItemSpacing() const { return m_itemSpacing; }

    void setMargins(float left, float top, float right, float bottom) { m_margins = {left, top, right, bottom}; }

    void setMargins(float margin) { setMargins(margin, margin, margin, margin); }

    [[nodiscard]] const ImVec4& getMargins() const { return m_margins; }

    void setDefaultItemHeight(float height) { m_defaultItemHeight = std::max(0.0F, height); }
    [[nodiscard]] float getDefaultItemHeight() const { return m_defaultItemHeight; }

    // ===================== 滚动条配置 =====================
    void setScrollbarEnabled(bool enabled) { m_scrollbarEnabled = enabled; }
    [[nodiscard]] bool isScrollbarEnabled() const { return m_scrollbarEnabled; }

    // ===================== Widget 访问 =====================
    [[nodiscard]] Widget::Ptr getWidget(size_t index) const
    {
        if (index >= m_items.size())
        {
            return nullptr;
        }
        return m_items[index].widget;
    }

    [[nodiscard]] const std::vector<ListItem>& getItems() const { return m_items; }

    // ===================== 尺寸计算 =====================
    ImVec2 calculateSize() override
    {
        if (isFixedSize())
        {
            return Widget::calculateSize();
        }

        float totalSize = 0.0F;
        float maxCrossSize = 0.0F;
        size_t visibleCount = 0;

        for (const auto& item : m_items)
        {
            if (!item.widget || !item.widget->isVisible())
            {
                continue;
            }

            ImVec2 widgetSize = item.widget->calculateSize();

            float itemHeight = item.customHeight > 0.0F ? item.customHeight : m_defaultItemHeight;

            if (m_direction == ListDirection::VERTICAL)
            {
                float height = itemHeight > 0.0F ? itemHeight : widgetSize.y;
                totalSize += height;
                maxCrossSize = std::max(maxCrossSize, widgetSize.x);
            }
            else
            {
                float width = widgetSize.x;
                totalSize += width;
                maxCrossSize = std::max(maxCrossSize, itemHeight > 0.0F ? itemHeight : widgetSize.y);
            }
            ++visibleCount;
        }

        // 添加间距
        if (visibleCount > 1)
        {
            totalSize += m_itemSpacing * static_cast<float>(visibleCount - 1);
        }

        // 添加边距
        float width = 0.0F;
        float height = 0.0F;
        if (m_direction == ListDirection::VERTICAL)
        {
            width = maxCrossSize + m_margins.x + m_margins.z;
            height = totalSize + m_margins.y + m_margins.w;
        }
        else
        {
            width = totalSize + m_margins.x + m_margins.z;
            height = maxCrossSize + m_margins.y + m_margins.w;
        }

        width = std::clamp(width, getMinWidth(), getMaxWidth());
        height = std::clamp(height, getMinHeight(), getMaxHeight());
        return {width, height};
    }

protected:
    static constexpr float HALF = 0.5F;
    static constexpr float DEFAULT_SPACING = 5.0F;
    static constexpr uint32_t SELECTION_BG_COLOR = 0x64649BFF; // RGBA: 100, 100, 155, 255

    void onRender(const ImVec2& position, const ImVec2& size) override
    {
        if (m_items.empty())
        {
            return;
        }

        // 计算内容区域
        float contentX = position.x + m_margins.x;
        float contentY = position.y + m_margins.y;
        float contentWidth = size.x - m_margins.x - m_margins.z;
        float contentHeight = size.y - m_margins.y - m_margins.w;

        // 确保内容区域不为负
        contentWidth = std::max(0.0f, contentWidth);
        contentHeight = std::max(0.0f, contentHeight);

        ImVec2 contentSize(contentWidth, contentHeight);

        if (m_scrollbarEnabled)
        {
            // 启用滚动条：创建子窗口
            ImGui::SetCursorPos(ImVec2(contentX, contentY));

            // 根据方向设置滚动条标志
            ImGuiWindowFlags scrollFlags = ImGuiWindowFlags_None;
            if (m_direction == ListDirection::VERTICAL)
            {
                scrollFlags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
                if (contentWidth > 0)
                {
                    scrollFlags |= ImGuiWindowFlags_AlwaysHorizontalScrollbar;
                }
            }
            else
            {
                scrollFlags |= ImGuiWindowFlags_AlwaysHorizontalScrollbar;
                if (contentHeight > 0)
                {
                    scrollFlags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
                }
            }

            ImGui::BeginChild("##ListAreaScroll", contentSize, ImGuiChildFlags_None, scrollFlags);

            // 在子窗口内渲染
            ImVec2 childContentSize = ImGui::GetContentRegionAvail();
            renderItems(ImVec2(0, 0), childContentSize);

            ImGui::EndChild();
        }
        else
        {
            // 无滚动条：直接渲染
            ImGui::SetCursorPos(ImVec2(contentX, contentY));
            renderItems(ImVec2(contentX, contentY), contentSize);
        }
    }

private:
    void renderItems(const ImVec2& basePos, const ImVec2& availableSize)
    {
        // 计算所有可见项的总尺寸
        float totalItemSize = 0.0F;
        size_t visibleCount = 0;
        for (const auto& item : m_items)
        {
            if (!item.widget || !item.widget->isVisible())
            {
                continue;
            }

            ImVec2 widgetSize = item.widget->calculateSize();
            float itemHeight = item.customHeight > 0.0F ? item.customHeight : m_defaultItemHeight;

            if (m_direction == ListDirection::VERTICAL)
            {
                totalItemSize += (itemHeight > 0.0F ? itemHeight : widgetSize.y);
            }
            else
            {
                totalItemSize += widgetSize.x;
            }
            ++visibleCount;
        }

        if (visibleCount > 1)
        {
            totalItemSize += m_itemSpacing * static_cast<float>(visibleCount - 1);
        }

        // 计算起始位置（考虑对齐）
        ImVec2 startPos = basePos;
        if (m_direction == ListDirection::VERTICAL)
        {
            float offset = calculateAlignmentOffset(availableSize.y, totalItemSize);
            startPos.y += offset;
        }
        else
        {
            float offset = calculateAlignmentOffset(availableSize.x, totalItemSize);
            startPos.x += offset;
        }

        ImVec2 currentPos = startPos;

        // 渲染每个项
        for (size_t i = 0; i < m_items.size(); ++i)
        {
            auto& item = m_items[i];
            if (!item.widget || !item.widget->isVisible())
            {
                continue;
            }

            renderSingleItem(item, i, currentPos, availableSize);
        }
    }

    [[nodiscard]] float calculateAlignmentOffset(float availableSpace, float totalSize) const
    {
        // 如果内容超过可用空间，或者对齐方式是START，则从0开始
        if (totalSize > availableSpace || m_alignment == ListAlignment::START)
        {
            return 0.0F;
        }

        if (m_alignment == ListAlignment::CENTER)
        {
            return (availableSpace - totalSize) * HALF;
        }

        if (m_alignment == ListAlignment::END)
        {
            return availableSpace - totalSize;
        }

        return 0.0F;
    }

    void renderSingleItem(ListItem& item, size_t index, ImVec2& currentPos, const ImVec2& availableSize)
    {
        ImVec2 widgetSize = item.widget->calculateSize();
        ImVec2 renderSize = calculateRenderSize(widgetSize, item.customHeight, availableSize);

        // 获取项的实际屏幕位置
        ImVec2 itemScreenPos;
        if (m_scrollbarEnabled)
        {
            // 在子窗口内，使用GetCursorScreenPos获取屏幕坐标
            ImGui::SetCursorPos(currentPos);
            itemScreenPos = ImGui::GetCursorScreenPos();
        }
        else
        {
            // 不在子窗口内，需要加上窗口位置
            itemScreenPos = ImGui::GetWindowPos();
            itemScreenPos.x += currentPos.x;
            itemScreenPos.y += currentPos.y;
        }

        // 绘制选中背景（在渲染widget之前）
        if (item.selected && m_selectionMode != SelectionMode::NONE)
        {
            drawSelectionBackground(itemScreenPos, renderSize);
        }

        // 渲染widget
        ImGui::SetCursorPos(currentPos);
        item.widget->render(currentPos, renderSize);

        // 处理点击事件
        if (m_selectionMode != SelectionMode::NONE && ImGui::IsItemClicked())
        {
            handleItemClick(index);
        }

        // 移动到下一个位置
        advancePosition(currentPos, renderSize);
    }

    [[nodiscard]] ImVec2
        calculateRenderSize(const ImVec2& widgetSize, float customHeight, const ImVec2& availableSize) const
    {
        ImVec2 renderSize = widgetSize;
        float itemHeight = customHeight > 0.0F ? customHeight : m_defaultItemHeight;

        if (m_direction == ListDirection::VERTICAL)
        {
            // 垂直方向：宽度填充可用宽度，高度使用自定义高度或widget高度
            renderSize.x = std::max(0.0F, availableSize.x);
            if (itemHeight > 0.0F)
            {
                renderSize.y = itemHeight;
            }
        }
        else
        {
            // 水平方向：宽度使用widget宽度，高度使用自定义高度或可用高度
            if (itemHeight > 0.0F)
            {
                renderSize.y = itemHeight;
            }
            else
            {
                renderSize.y = std::max(0.0F, availableSize.y);
            }
        }

        return renderSize;
    }

    static void drawSelectionBackground(const ImVec2& screenPos, const ImVec2& size)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 bottomRight(screenPos.x + size.x, screenPos.y + size.y);
        drawList->AddRectFilled(screenPos, bottomRight, SELECTION_BG_COLOR);
    }

    void advancePosition(ImVec2& currentPos, const ImVec2& renderSize)
    {
        if (m_direction == ListDirection::VERTICAL)
        {
            currentPos.y += renderSize.y + m_itemSpacing;
        }
        else
        {
            currentPos.x += renderSize.x + m_itemSpacing;
        }
    }

    void handleItemClick(size_t index)
    {
        if (index >= m_items.size())
        {
            return;
        }

        if (m_selectionMode == SelectionMode::SINGLE)
        {
            // 单选模式
            if (m_items[index].selected)
            {
                // 如果已选中，取消选择
                m_items[index].selected = false;
            }
            else
            {
                // 清除其他选择，选中当前项
                clearSelection();
                m_items[index].selected = true;
            }
        }
        else if (m_selectionMode == SelectionMode::MULTIPLE)
        {
            // 多选模式：切换选择状态
            m_items[index].selected = !m_items[index].selected;
        }
    }

    std::vector<ListItem> m_items;
    ListDirection m_direction = ListDirection::VERTICAL;
    ListAlignment m_alignment = ListAlignment::START;
    SelectionMode m_selectionMode = SelectionMode::NONE;
    float m_itemSpacing = DEFAULT_SPACING;
    float m_defaultItemHeight = 0.0F;         // 0 表示使用widget自身高度
    ImVec4 m_margins{0.0F, 0.0F, 0.0F, 0.0F}; // left, top, right, bottom
    bool m_scrollbarEnabled = true;
};

} // namespace ui