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
#include "Widget.h"
#include "Button.h"
#include "ListArea.h"
class OperationArea : public ui::Widget
{
public:
    OperationArea()
    {
        m_topListArea = std::make_shared<ui::ListArea>(ui::ListDirection::HORIZONTAL);
        m_topListArea->setAlignment(ui::Alignment::CENTER);
        m_topListArea->setSpacing(10);
        m_topListArea->addWidget(std::make_shared<ui::Button>("使用卡牌"));
        m_topListArea->addWidget(std::make_shared<ui::Button>("取消"));
        m_topListArea->addWidget(std::make_shared<ui::Button>("结束回合"));
        m_topListArea->setPosition(0, 0);
        addChild(m_topListArea);
        m_bottomListArea = std::make_shared<ui::ListArea>(ui::ListDirection::HORIZONTAL);
        m_bottomListArea->setAlignment(ui::Alignment::LEFT);
        m_bottomListArea->setSpacing(10);
        m_bottomListArea->setPosition(0, 40);
        addChild(m_bottomListArea);
    }
    void addCardToHand(const std::string& cardName)
    {
        m_bottomListArea->addWidget(std::make_shared<ui::Label>(cardName));
    }

private:
    std::shared_ptr<ui::ListArea> m_topListArea;
    std::shared_ptr<ui::ListArea> m_bottomListArea;
};