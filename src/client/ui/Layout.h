/**
 * ************************************************************************
 *
 * @file Layout.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-11-27
 * @version 0.1
 * @brief 布局容器定义
  模拟 Qt 的布局容器 QLayout、QHBoxLayout、QVBoxLayout

  使用示例：
  1. addStretch() - 添加弹性空间（推荐用于简单场景）
  2. addWidget(createHorizontalSpacer()) - 添加 Spacer 组件（更灵活）
  3. addWidget(createFixedSpacer(20, 10)) - 添加固定尺寸空白
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <cstddef>
#include <memory>
#include <vector>
#include <algorithm>
#include "Widget.h"
#include <sys/stat.h>

namespace ui
{
enum class LayoutDirection : uint8_t
{
    HORIZONTAL,
    VERTICAL
};
enum class Alignment : uint8_t
{
    LEFT = 0x01,
    RIGHT = 0x02,
    TOP = 0x04,
    BOTTOM = 0x08,
    H_CENTER = 0x10,
    V_CENTER = 0x20,
    CENTER = H_CENTER | V_CENTER
};

struct LayoutItem
{
    std::shared_ptr<Widget> widget;
    int stretchFactor = 0;
    Alignment alignment = Alignment::LEFT;
};

class Layout : public Widget
{
public:
    Layout() { setBackgroundEnabled(false); }

    void addWidget(const std::shared_ptr<Widget>& widget, int stretch = 0, Alignment alignment = Alignment::LEFT)
    {
        if (!widget)
        {
            return;
        }
        m_items.push_back({widget, stretch, alignment});
    }
    void addStretch(int stretch = 1) { m_items.push_back({nullptr, stretch, Alignment::LEFT}); }
    void removeWidget(const std::shared_ptr<Widget>& widget)
    {
        m_items.erase(std::remove_if(m_items.begin(),
                                     m_items.end(),
                                     [&widget](const LayoutItem& item) { return item.widget == widget; }),
                      m_items.end());
    }
    void clear() { m_items.clear(); }
    void setDirection(LayoutDirection direction) { m_direction = direction; }
    void setSpacing(float spacing) { m_spacing = std::max(0.0F, spacing); }
    void setMargins(float left, float top, float right, float bottom) { m_margins = {left, top, right, bottom}; }
    void setMargins(float margins) { setMargins(margins, margins, margins, margins); }
    [[nodiscard]] const std::vector<LayoutItem>& getItems() const { return m_items; }
    [[nodiscard]] size_t count() const { return m_items.size(); }
    [[nodiscard]] const ImVec4& getMargins() const { return m_margins; }
    [[nodiscard]] float getSpacing() const { return m_spacing; }
    [[nodiscard]] LayoutDirection getDirection() const { return m_direction; }

protected:
    [[nodiscard]] std::vector<LayoutItem>& items() { return m_items; }
    [[nodiscard]] const std::vector<LayoutItem>& items() const { return m_items; }
    [[nodiscard]] const ImVec4& margins() const { return m_margins; }
    [[nodiscard]] float spacing() const { return m_spacing; }
    static constexpr float HALF = 0.5F;

    [[nodiscard]] static bool hasAlignment(Alignment value, Alignment flag)
    {
        return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
    }

private:
    std::vector<LayoutItem> m_items;
    LayoutDirection m_direction = LayoutDirection::HORIZONTAL;
    float m_spacing = 5.0F;
    ImVec4 m_margins{0, 0, 0, 0};
};

// ------------------ 水平布局 ------------------
class HBoxLayout : public Layout
{
public:
    HBoxLayout()
    {
        setDirection(LayoutDirection::HORIZONTAL);
        setBackgroundEnabled(false);
    }

protected:
    void onRender(const ImVec2& position, const ImVec2& size) override
    {
        if (items().empty())
        {
            return;
        }

        ImVec2 contentPos(position.x + margins().x, position.y + margins().y);
        float availableWidth = size.x - margins().x - margins().z;
        float availableHeight = size.y - margins().y - margins().w;

        // spacing 只计算组件之间的间距(N个组件有N-1个间距)
        size_t widgetCount = std::count_if(
            items().begin(), items().end(), [](const LayoutItem& item) { return item.widget != nullptr; });
        float totalSpacing = widgetCount > 1 ? spacing() * static_cast<float>(widgetCount - 1) : 0.0F;

        float totalFixedWidth = 0;
        int totalStretch = 0;
        std::vector<float> widths(items().size());

        for (size_t i = 0; i < items().size(); ++i)
        {
            auto& item = items()[i];
            if (!item.widget)
            {
                totalStretch += item.stretchFactor;
                widths[i] = 0;
            }
            else if (item.widget->isFixedSize() || item.stretchFactor == 0)
            {
                widths[i] = item.widget->calculateSize().x;
                totalFixedWidth += widths[i];
            }
            else
            {
                widths[i] = 0;
                totalStretch += item.stretchFactor;
            }
        }

        float remainingWidth = availableWidth - totalFixedWidth - totalSpacing;
        if (remainingWidth > 0 && totalStretch > 0)
        {
            for (size_t i = 0; i < items().size(); ++i)
            {
                auto& item = items()[i];
                if (item.stretchFactor > 0)
                {
                    widths[i] =
                        remainingWidth * static_cast<float>(item.stretchFactor) / static_cast<float>(totalStretch);
                }
            }
        }

        float currentX = contentPos.x;
        for (size_t i = 0; i < items().size(); ++i)
        {
            auto& item = items()[i];
            if (!item.widget)
            {
                currentX += widths[i];
                continue;
            }

            ImVec2 itemPos(currentX, contentPos.y);
            ImVec2 itemSize(widths[i], availableHeight);

            // 垂直对齐
            if (hasAlignment(item.alignment, Alignment::V_CENTER))
            {
                ImVec2 pref = item.widget->calculateSize();
                itemPos.y += (availableHeight - pref.y) * HALF;
                itemSize.y = pref.y;
            }
            else if (hasAlignment(item.alignment, Alignment::BOTTOM))
            {
                ImVec2 pref = item.widget->calculateSize();
                itemPos.y += availableHeight - pref.y;
                itemSize.y = pref.y;
            }

            item.widget->render(itemPos, itemSize);
            currentX += widths[i];
            if (i < items().size() - 1)
            {
                currentX += spacing();
            }
        }
    }

public:
    ImVec2 calculateSize() override
    {
        if (items().empty())
        {
            return {margins().x + margins().z, margins().y + margins().w};
        }

        float totalWidth = margins().x + margins().z;
        float maxHeight = 0;
        size_t widgetCount = 0;

        for (auto& item : items())
        {
            if (!item.widget)
            {
                continue;
            }
            ImVec2 size = item.widget->calculateSize();
            totalWidth += size.x;
            maxHeight = std::max(maxHeight, size.y);
            widgetCount++;
        }

        if (widgetCount > 1)
        {
            totalWidth += spacing() * static_cast<float>(widgetCount - 1);
        }
        return {totalWidth, maxHeight + margins().y + margins().w};
    }
};

// ------------------ 垂直布局 ------------------
class VBoxLayout : public Layout
{
public:
    VBoxLayout()
    {
        setDirection(LayoutDirection::VERTICAL);
        setBackgroundEnabled(false);
    }

protected:
    void onRender(const ImVec2& position, const ImVec2& size) override
    {
        if (items().empty())
        {
            return;
        }

        ImVec2 contentPos(position.x + margins().x, position.y + margins().y);
        float availableWidth = size.x - margins().x - margins().z;
        float availableHeight = size.y - margins().y - margins().w;

        // spacing 只计算组件之间的间距(N个组件有N-1个间距)
        size_t widgetCount = std::count_if(
            items().begin(), items().end(), [](const LayoutItem& item) { return item.widget != nullptr; });
        float totalSpacing = widgetCount > 1 ? spacing() * static_cast<float>(widgetCount - 1) : 0.0F;

        float totalFixedHeight = 0;
        int totalStretch = 0;
        std::vector<float> heights(items().size());

        for (size_t i = 0; i < items().size(); ++i)
        {
            auto& item = items()[i];
            if (!item.widget)
            {
                totalStretch += item.stretchFactor;
                heights[i] = 0;
            }
            else if (item.widget->isFixedSize() || item.stretchFactor == 0)
            {
                heights[i] = item.widget->calculateSize().y;
                totalFixedHeight += heights[i];
            }
            else
            {
                heights[i] = 0;
                totalStretch += item.stretchFactor;
            }
        }

        float remainingHeight = availableHeight - totalFixedHeight - totalSpacing;
        if (remainingHeight > 0 && totalStretch > 0)
        {
            for (size_t i = 0; i < items().size(); ++i)
            {
                auto& item = items()[i];
                if (item.stretchFactor > 0)
                {
                    heights[i] =
                        remainingHeight * static_cast<float>(item.stretchFactor) / static_cast<float>(totalStretch);
                }
            }
        }

        float currentY = contentPos.y;
        for (size_t i = 0; i < items().size(); ++i)
        {
            auto& item = items()[i];
            if (!item.widget)
            {
                currentY += heights[i];
                continue;
            }

            ImVec2 itemPos(contentPos.x, currentY);
            ImVec2 itemSize(availableWidth, heights[i]);

            // 水平对齐
            if (hasAlignment(item.alignment, Alignment::H_CENTER))
            {
                ImVec2 pref = item.widget->calculateSize();
                itemPos.x += (availableWidth - pref.x) * HALF;
                itemSize.x = pref.x;
            }
            else if (hasAlignment(item.alignment, Alignment::RIGHT))
            {
                ImVec2 pref = item.widget->calculateSize();
                itemPos.x += availableWidth - pref.x;
                itemSize.x = pref.x;
            }

            item.widget->render(itemPos, itemSize);
            currentY += heights[i];
            if (i < items().size() - 1)
            {
                currentY += spacing();
            }
        }
    }

public:
    ImVec2 calculateSize() override
    {
        if (items().empty())
        {
            return {margins().x + margins().z, margins().y + margins().w};
        }

        float maxWidth = 0;
        float totalHeight = margins().y + margins().w;
        size_t widgetCount = 0;

        for (auto& item : items())
        {
            if (!item.widget)
            {
                continue;
            }
            ImVec2 size = item.widget->calculateSize();
            maxWidth = std::max(maxWidth, size.x);
            totalHeight += size.y;
            widgetCount++;
        }

        if (widgetCount > 1)
        {
            totalHeight += spacing() * static_cast<float>(widgetCount - 1);
        }
        return {maxWidth + margins().x + margins().z, totalHeight};
    }
};
} // namespace ui