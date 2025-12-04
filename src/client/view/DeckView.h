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
#include "Widget.h"
#include "label.h"

class DeckView : public ui::Widget
{
public:
    DeckView()
    {
        m_deckLabel = std::make_shared<ui::Label>("Deck: 0");
        m_discardLabel = std::make_shared<ui::Label>("Discard: 0");

        m_deckLabel->setPosition(0, 0);
        m_discardLabel->setPosition(0, 20);

        addChild(m_deckLabel);
        addChild(m_discardLabel);
    }

    void setDeckCount(int count) { m_deckLabel->setText("Deck: " + std::to_string(count)); }

    void setDiscardCount(int count) { m_discardLabel->setText("Discard: " + std::to_string(count)); }

private:
    std::shared_ptr<ui::Label> m_deckLabel;
    std::shared_ptr<ui::Label> m_discardLabel;
};