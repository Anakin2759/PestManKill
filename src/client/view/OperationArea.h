/**
 * ************************************************************************
 *
 * @file OperationArea.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-04
 * @version 0.1
 * @brief 操作区视图组件
    一个横向的长方形区域
    上方有一个横向的ListArea,居中对齐
    包含按钮：使用卡牌，取消，结束回合
    下方有一个横向的ListArea,左对齐
    显示当前手牌列表
   - 包含方法添加手牌显示 HandCardView
   - 提供方法把手牌移动到处理区 ，并从手牌区移除，移动过程中有动画效果
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include "src/client/ui/Widget.h"
#include "src/client/ui/Button.h"
#include "src/client/ui/ListArea.h"
#include "src/client/ui/Layout.h"
#include "HandCardView.h"
#include <memory>
#include <vector>
#include <functional>
#include <algorithm>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

/**
 * @brief 操作区视图组件
 * 显示操作按钮和手牌区域
 */
class OperationArea : public ui::Widget
{
public:
    OperationArea() { setupUI(); }

    ~OperationArea() override = default;

    // ===================== 手牌管理 =====================
    void addHandCard(std::shared_ptr<HandCardView> card)
    {
        if (!card)
        {
            return;
        }

        m_handCards.push_back(card);
        m_handCardArea->addWidget(card);

        // 设置卡牌选择回调
        card->setOnSelectCallback(
            [this, card](bool selected)
            {
                if (selected && m_onCardSelected)
                {
                    m_onCardSelected(card);
                }
                else if (!selected && m_onCardDeselected)
                {
                    m_onCardDeselected(card);
                }
            });
    }

    void removeHandCard(std::shared_ptr<HandCardView> card)
    {
        if (!card)
        {
            return;
        }

        auto it = std::find(m_handCards.begin(), m_handCards.end(), card);
        if (it != m_handCards.end())
        {
            m_handCards.erase(it);
            m_handCardArea->removeWidget(card);
        }
    }

    void clearHandCards()
    {
        m_handCards.clear();
        m_handCardArea->clear();
    }

    [[nodiscard]] size_t getHandCardCount() const { return m_handCards.size(); }

    [[nodiscard]] const std::vector<std::shared_ptr<HandCardView>>& getHandCards() const { return m_handCards; }

    std::shared_ptr<HandCardView> getSelectedCard() const
    {
        for (const auto& card : m_handCards)
        {
            if (card->isSelected())
            {
                return card;
            }
        }
        return nullptr;
    }

    std::vector<std::shared_ptr<HandCardView>> getSelectedCards() const
    {
        std::vector<std::shared_ptr<HandCardView>> selected;
        for (const auto& card : m_handCards)
        {
            if (card->isSelected())
            {
                selected.push_back(card);
            }
        }
        return selected;
    }

    // ===================== 按钮回调设置 =====================
    void setOnUseCard(std::function<void()> callback) { m_useCardBtn->setOnClick(std::move(callback)); }

    void setOnCancel(std::function<void()> callback) { m_cancelBtn->setOnClick(std::move(callback)); }

    void setOnEndTurn(std::function<void()> callback) { m_endTurnBtn->setOnClick(std::move(callback)); }

    void setOnCardSelected(std::function<void(std::shared_ptr<HandCardView>)> callback)
    {
        m_onCardSelected = std::move(callback);
    }

    void setOnCardDeselected(std::function<void(std::shared_ptr<HandCardView>)> callback)
    {
        m_onCardDeselected = std::move(callback);
    }

    void setOnCardMoveToProcessing(std::function<void(std::shared_ptr<HandCardView>)> callback)
    {
        m_onCardMoveToProcessing = std::move(callback);
    }

    // ===================== 按钮状态控制 =====================
    void setUseCardEnabled(bool enabled) { m_useCardBtn->setEnabled(enabled); }

    void setCancelEnabled(bool enabled) { m_cancelBtn->setEnabled(enabled); }

    void setEndTurnEnabled(bool enabled) { m_endTurnBtn->setEnabled(enabled); }

    // ===================== 移动卡牌到处理区 =====================
    void moveSelectedCardToProcessing()
    {
        auto selected = getSelectedCard();
        if (selected && m_onCardMoveToProcessing)
        {
            m_onCardMoveToProcessing(selected);
            removeHandCard(selected);
        }
    }

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
        m_mainLayout->setMargins(10, 10, 10, 10);
        m_mainLayout->setSpacing(10);

        // 上方：操作按钮区域
        m_buttonArea = std::make_shared<ui::ListArea>(ui::ListDirection::HORIZONTAL);
        m_buttonArea->setAlignment(ui::ListAlignment::CENTER);
        m_buttonArea->setScrollbarEnabled(false);
        m_buttonArea->setItemSpacing(10.0F);

        m_useCardBtn = std::make_shared<ui::Button>("使用卡牌");
        m_useCardBtn->setFixedSize(120, 40);
        m_buttonArea->addWidget(m_useCardBtn);

        m_cancelBtn = std::make_shared<ui::Button>("取消");
        m_cancelBtn->setFixedSize(100, 40);
        m_buttonArea->addWidget(m_cancelBtn);

        m_endTurnBtn = std::make_shared<ui::Button>("结束回合");
        m_endTurnBtn->setFixedSize(120, 40);
        m_buttonArea->addWidget(m_endTurnBtn);

        m_mainLayout->addWidget(m_buttonArea, 0);

        // 下方：手牌区域
        m_handCardArea = std::make_shared<ui::ListArea>(ui::ListDirection::HORIZONTAL);
        m_handCardArea->setAlignment(ui::ListAlignment::START);
        m_handCardArea->setScrollbarEnabled(true);
        m_handCardArea->setItemSpacing(8.0F);
        m_handCardArea->setMargins(5.0F);
        m_handCardArea->setBackgroundEnabled(true);
        m_handCardArea->setBackgroundColor(ImVec4(0.1F, 0.12F, 0.15F, 0.8F));

        m_mainLayout->addWidget(m_handCardArea, 1);

        addChild(m_mainLayout);
    }

    // 成员变量
    std::shared_ptr<ui::VBoxLayout> m_mainLayout;
    std::shared_ptr<ui::ListArea> m_buttonArea;
    std::shared_ptr<ui::ListArea> m_handCardArea;

    std::shared_ptr<ui::Button> m_useCardBtn;
    std::shared_ptr<ui::Button> m_cancelBtn;
    std::shared_ptr<ui::Button> m_endTurnBtn;

    std::vector<std::shared_ptr<HandCardView>> m_handCards;

    // 回调函数
    std::function<void(std::shared_ptr<HandCardView>)> m_onCardSelected;
    std::function<void(std::shared_ptr<HandCardView>)> m_onCardDeselected;
    std::function<void(std::shared_ptr<HandCardView>)> m_onCardMoveToProcessing;
};

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)