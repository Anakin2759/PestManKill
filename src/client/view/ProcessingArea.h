/**
 * ************************************************************************
 *
 * @file ProcessingArea.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-04
 * @version 0.1
 * @brief 处理区视图组件
    一个宽长方形区域
    显示在游戏界面的中央位置
    用于显示当前回合的处理区内容
    包含玩家打出的卡牌、技能效果等信息

    底层是一个横向布局的ListArea
    显示当前处理区内的卡牌视图 HandCardView
    卡牌移动到处理区有平移动画效果，相对于手牌区的位置移动
    清空时卡牌有淡出动画效果，发出移动到弃牌堆的事件
    - 提供方法添加卡牌视图 HandCardView
    - 提供方法移除所有卡牌视图 HandCardView
    - 提供方法响应卡牌移动到处理区事件
    - 提供方法响应清空处理区事件

 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include "src/client/ui/Widget.h"
#include "src/client/ui/ListArea.h"
#include "src/client/ui/Label.h"
#include "HandCardView.h"
#include <memory>
#include <vector>
#include <functional>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

/**
 * @brief 处理区视图组件
 * 显示当前回合打出的卡牌
 */
class ProcessingArea : public ui::Widget
{
public:
    ProcessingArea() { setupUI(); }

    ~ProcessingArea() override = default;

    // ===================== 卡牌管理 =====================
    void addCard(std::shared_ptr<HandCardView> card)
    {
        if (!card)
        {
            return;
        }

        m_cards.push_back(card);
        m_cardArea->addWidget(card);

        // 触发卡牌添加回调
        if (m_onCardAdded)
        {
            m_onCardAdded(card);
        }
    }

    void removeCard(std::shared_ptr<HandCardView> card)
    {
        if (!card)
        {
            return;
        }

        auto it = std::find(m_cards.begin(), m_cards.end(), card);
        if (it != m_cards.end())
        {
            m_cards.erase(it);
            m_cardArea->removeWidget(card);

            // 触发卡牌移除回调
            if (m_onCardRemoved)
            {
                m_onCardRemoved(card);
            }
        }
    }

    void clearAllCards()
    {
        // 触发清空前回调（可以在这里播放动画）
        if (m_onBeforeClear)
        {
            m_onBeforeClear(m_cards);
        }

        m_cards.clear();
        m_cardArea->clear();

        // 触发清空后回调
        if (m_onAfterClear)
        {
            m_onAfterClear();
        }
    }

    void clear() { clearAllCards(); }

    [[nodiscard]] size_t getCardCount() const { return m_cards.size(); }

    [[nodiscard]] const std::vector<std::shared_ptr<HandCardView>>& getCards() const { return m_cards; }

    // ===================== 回调设置 =====================
    void setOnCardAdded(std::function<void(std::shared_ptr<HandCardView>)> callback)
    {
        m_onCardAdded = std::move(callback);
    }

    void setOnCardRemoved(std::function<void(std::shared_ptr<HandCardView>)> callback)
    {
        m_onCardRemoved = std::move(callback);
    }

    void setOnBeforeClear(std::function<void(const std::vector<std::shared_ptr<HandCardView>>&)> callback)
    {
        m_onBeforeClear = std::move(callback);
    }

    void setOnAfterClear(std::function<void()> callback) { m_onAfterClear = std::move(callback); }

    // ===================== 样式设置 =====================
    void setTitle(const std::string& title) { m_titleLabel->setText(title); }

    void showTitle(bool show) { m_titleLabel->setVisible(show); }

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

            // 绘制边框
            drawList->AddRect(position, bottomRight, IM_COL32(80, 80, 100, 200), 6.0F, 0, 2.0F);
        }

        // 渲染主布局
        m_mainLayout->render(position, size);
    }

private:
    void setupUI()
    {
        setBackgroundEnabled(true);
        setBackgroundColor(ImVec4(0.15F, 0.17F, 0.20F, 0.95F));

        m_mainLayout = std::make_shared<ui::VBoxLayout>();
        m_mainLayout->setMargins(10, 10, 10, 10);
        m_mainLayout->setSpacing(8);

        // 标题
        m_titleLabel = std::make_shared<ui::Label>("处理区");
        m_titleLabel->setTextColor(ImVec4(1.0F, 0.9F, 0.3F, 1.0F));
        m_titleLabel->setBackgroundEnabled(false);
        m_mainLayout->addWidget(m_titleLabel, 0);

        // 卡牌区域（横向列表）
        m_cardArea = std::make_shared<ui::ListArea>(ui::ListDirection::HORIZONTAL);
        m_cardArea->setScrollbarEnabled(true);
        m_cardArea->setAlignment(ui::ListAlignment::CENTER);
        m_cardArea->setItemSpacing(10.0F);
        m_cardArea->setMargins(5.0F);
        m_cardArea->setBackgroundEnabled(true);
        m_cardArea->setBackgroundColor(ImVec4(0.1F, 0.12F, 0.15F, 0.8F));

        m_mainLayout->addWidget(m_cardArea, 1);

        addChild(m_mainLayout);
    }

    // 成员变量
    std::shared_ptr<ui::VBoxLayout> m_mainLayout;
    std::shared_ptr<ui::Label> m_titleLabel;
    std::shared_ptr<ui::ListArea> m_cardArea;

    std::vector<std::shared_ptr<HandCardView>> m_cards;

    // 回调函数
    std::function<void(std::shared_ptr<HandCardView>)> m_onCardAdded;
    std::function<void(std::shared_ptr<HandCardView>)> m_onCardRemoved;
    std::function<void(const std::vector<std::shared_ptr<HandCardView>>&)> m_onBeforeClear;
    std::function<void()> m_onAfterClear;
};

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
