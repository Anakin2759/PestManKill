/**
 * ************************************************************************
 *
 * @file HandCardView.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-04
 * @version 0.1
 * @brief 手牌视图组件
    一个竖向的长方形区域
    是玩家手牌的其中一张的显示区域
    显示卡牌图案、名称、类型等信息
    可以点击选中卡牌、再次点击取消选中
    左上角显示卡牌的花色和点数（暂时是文字）
    居中显示卡牌图案（暂无）
    卡牌选中自动触发选择目标事件、取消选中触发取消选择事件
    -提供显示卡牌信息的方法 show
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include "src/client/ui/Widget.h"
#include "src/client/ui/Image.h"
#include "src/client/ui/Label.h"

class HandCardView : public ui::Widget
{
public:
    HandCardView()
    {
        m_cardImage = std::make_shared<ui::Image>();
        m_cardNameLabel = std::make_shared<ui::Label>("Card Name");
        m_suitLabel = std::make_shared<ui::Label>("黑桃A");
        m_suitLabel->setBackgroundEnabled(false);
        setBackgroundEnabled(true);
        setFixedSize(95, 135);
        m_selected = false;
    }

    ~HandCardView() override = default;

    // 显示卡牌信息
    void show(std::string_view name, std::string_view suitText)
    {
        m_cardName = name;
        m_cardNameLabel->setText(std::string{name});
        m_suitLabel->setText(std::string{suitText});
    }

    // 切换选中状态
    void toggleSelect() { m_selected = !m_selected; }

    bool isSelected() const { return m_selected; }

    // 点击事件处理（外部可调用）
    void onClick()
    {
        toggleSelect();
        if (m_onSelect)
        {
            m_onSelect(m_selected);
        }
    }

    void setOnSelectCallback(std::function<void(bool)> cb) { m_onSelect = std::move(cb); }

protected:
    ImVec2 calculateSize() override
    {
        if (isFixedSize())
        {
            return Widget::calculateSize();
        }
        return {95.0F, 135.0F};
    }

    void onRender(const ImVec2& position, const ImVec2& size) override
    {
        // 背景
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImU32 bg = m_selected ? IM_COL32(200, 180, 80, 255) : IM_COL32(28, 30, 34, 220);
        dl->AddRectFilled(ImVec2(position.x, position.y), ImVec2(position.x + size.x, position.y + size.y), bg, 6.0F);

        // 花色/点数（左上）
        ImVec2 suitPos = ImVec2(position.x + 6.0F, position.y + 6.0F);
        m_suitLabel->render(suitPos, ImVec2(40, 18));

        // 卡牌图像占位（居中）
        ImVec2 imgPos = ImVec2(position.x + (size.x - 60.0F) * 0.5F, position.y + 28.0F);
        m_cardImage->setFixedSize(60, 60);
        m_cardImage->render(imgPos, ImVec2(60, 60));

        // 名称（底部居中）
        ImVec2 namePos = ImVec2(position.x + 8.0F, position.y + size.y - 28.0F);
        m_cardNameLabel->render(namePos, ImVec2(size.x - 16.0F, 20.0F));
    }

private:
    std::shared_ptr<ui::Image> m_cardImage;
    std::shared_ptr<ui::Label> m_cardNameLabel;
    std::shared_ptr<ui::Label> m_suitLabel;
    std::string m_cardName;
    bool m_selected{false};
    std::function<void(bool)> m_onSelect;
};
