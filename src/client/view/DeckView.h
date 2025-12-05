/**
 * ************************************************************************
 *
 * @file DeckView.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-04
 * @version 0.1
 * @brief 卡组视图组件
    两组在两行显示
    [牌堆数量]
    [弃牌堆数量]

 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include "src/client/ui/Widget.h"
#include "src/client/ui/Label.h"
#include "src/client/ui/Layout.h"
#include "src/client/ui/Image.h"
#include <memory>
#include <string>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

/**
 * @brief 卡组视图组件
 * 显示牌堆和弃牌堆的数量
 */
class DeckView : public ui::Widget
{
public:
    DeckView() { setupUI(); }

    ~DeckView() override = default;

    // ===================== 设置数量 =====================
    void setDeckCount(int count)
    {
        m_deckCount = count;
        m_deckLabel->setText("牌堆: " + std::to_string(count));
    }

    void setDiscardCount(int count)
    {
        m_discardCount = count;
        m_discardLabel->setText("弃牌堆: " + std::to_string(count));
    }

    [[nodiscard]] int getDeckCount() const { return m_deckCount; }
    [[nodiscard]] int getDiscardCount() const { return m_discardCount; }

    // ===================== 样式设置 =====================
    void setLabelColor(const ImVec4& color)
    {
        m_deckLabel->setTextColor(color);
        m_discardLabel->setTextColor(color);
    }

    void setDeckIcon(const std::string& iconPath) { m_deckIcon->setImagePath(iconPath); }

    void setDiscardIcon(const std::string& iconPath) { m_discardIcon->setImagePath(iconPath); }

protected:
    ImVec2 calculateSize() override
    {
        if (isFixedSize())
        {
            return Widget::calculateSize();
        }
        return m_mainLayout->calculateSize();
    }

    void onRender(const ImVec2& position, const ImVec2& size) override
    {
        // 渲染背景
        if (isBackgroundEnabled())
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 bottomRight(position.x + size.x, position.y + size.y);
            drawList->AddRectFilled(position, bottomRight, ImGui::ColorConvertFloat4ToU32(getBackgroundColor()), 6.0F);
        }

        // 渲染主布局
        m_mainLayout->render(position, size);
    }

private:
    void setupUI()
    {
        setBackgroundEnabled(true);
        setBackgroundColor(ImVec4(0.12F, 0.14F, 0.17F, 0.9F));

        m_mainLayout = std::make_shared<ui::VBoxLayout>();
        m_mainLayout->setMargins(8, 8, 8, 8);
        m_mainLayout->setSpacing(8);

        // 牌堆行
        auto deckRow = std::make_shared<ui::HBoxLayout>();
        deckRow->setSpacing(6);

        m_deckIcon = std::make_shared<ui::Image>();
        m_deckIcon->setFixedSize(32, 32);
        m_deckIcon->setBackgroundEnabled(true);
        m_deckIcon->setBackgroundColor(ImVec4(0.2F, 0.3F, 0.5F, 0.9F));

        m_deckLabel = std::make_shared<ui::Label>("牌堆: 0");
        m_deckLabel->setTextColor(ImVec4(0.9F, 0.9F, 0.9F, 1.0F));
        m_deckLabel->setBackgroundEnabled(false);

        deckRow->addWidget(m_deckIcon, 0);
        deckRow->addWidget(m_deckLabel, 1);

        // 弃牌堆行
        auto discardRow = std::make_shared<ui::HBoxLayout>();
        discardRow->setSpacing(6);

        m_discardIcon = std::make_shared<ui::Image>();
        m_discardIcon->setFixedSize(32, 32);
        m_discardIcon->setBackgroundEnabled(true);
        m_discardIcon->setBackgroundColor(ImVec4(0.5F, 0.3F, 0.2F, 0.9F));

        m_discardLabel = std::make_shared<ui::Label>("弃牌堆: 0");
        m_discardLabel->setTextColor(ImVec4(0.9F, 0.9F, 0.9F, 1.0F));
        m_discardLabel->setBackgroundEnabled(false);

        discardRow->addWidget(m_discardIcon, 0);
        discardRow->addWidget(m_discardLabel, 1);

        m_mainLayout->addWidget(deckRow, 0);
        m_mainLayout->addWidget(discardRow, 0);

        addChild(m_mainLayout);
    }

    std::shared_ptr<ui::VBoxLayout> m_mainLayout;
    std::shared_ptr<ui::Image> m_deckIcon;
    std::shared_ptr<ui::Image> m_discardIcon;
    std::shared_ptr<ui::Label> m_deckLabel;
    std::shared_ptr<ui::Label> m_discardLabel;

    int m_deckCount{0};
    int m_discardCount{0};
};

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)