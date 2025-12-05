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

    - 提供设置卡牌图案、名称、花色、点数的方法
    - 提供选中和取消选中的方法

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
#include <string>
#include <functional>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

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
        m_hovered = false;
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

    void setCardImage(const std::string& imagePath) { m_cardImage->setImagePath(imagePath); }

    void setCardImage(const unsigned char* data, size_t size) { m_cardImage->loadFromMemory(data, size); }

    void setCardName(const std::string& name)
    {
        m_cardName = name;
        if (m_cardNameLabel)
        {
            m_cardNameLabel->setText(name);
        }
    }

    void setCardSuit(const std::string& suit)
    {
        m_suit = suit;
        updateCardInfo();
    }

    void setCardRank(const std::string& rank)
    {
        m_rank = rank;
        updateCardInfo();
    }

    [[nodiscard]] const std::string& getCardName() const { return m_cardName; }
    [[nodiscard]] const std::string& getSuit() const { return m_suit; }
    [[nodiscard]] const std::string& getRank() const { return m_rank; }
    [[nodiscard]] bool isHovered() const { return m_hovered; }

    void updateCardInfo()
    {
        // 更新显示（如果需要）
    }

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
        // 检测悬停
        ImVec2 mousePos = ImGui::GetMousePos();
        m_hovered = (mousePos.x >= position.x && mousePos.x <= position.x + size.x && mousePos.y >= position.y &&
                     mousePos.y <= position.y + size.y);

        // 背景颜色（选中/悬停/普通）
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImU32 bg = IM_COL32(28, 30, 34, 220);
        if (m_selected)
        {
            bg = IM_COL32(200, 180, 80, 255);
        }
        else if (m_hovered)
        {
            bg = IM_COL32(48, 50, 54, 240);
        }

        // 绘制阴影（悬停时更明显）
        if (m_hovered)
        {
            ImVec2 shadowOffset(2.0F, 2.0F);
            dl->AddRectFilled(ImVec2(position.x + shadowOffset.x, position.y + shadowOffset.y),
                              ImVec2(position.x + size.x + shadowOffset.x, position.y + size.y + shadowOffset.y),
                              IM_COL32(0, 0, 0, 100),
                              6.0F);
        }

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

        // 绘制边框
        ImU32 borderColor = m_selected  ? IM_COL32(255, 220, 100, 255)
                            : m_hovered ? IM_COL32(100, 150, 200, 200)
                                        : IM_COL32(60, 60, 60, 150);
        float borderThickness = m_selected ? 2.5F : (m_hovered ? 2.0F : 1.0F);
        dl->AddRect(ImVec2(position.x, position.y),
                    ImVec2(position.x + size.x, position.y + size.y),
                    borderColor,
                    6.0F,
                    0,
                    borderThickness);
    }

private:
    std::shared_ptr<ui::Image> m_cardImage;
    std::shared_ptr<ui::Label> m_cardNameLabel;
    std::shared_ptr<ui::Label> m_suitLabel;
    std::string m_cardName;
    std::string m_suit;
    std::string m_rank;
    bool m_selected{false};
    bool m_hovered{false};
    std::function<void(bool)> m_onSelect;
};

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
