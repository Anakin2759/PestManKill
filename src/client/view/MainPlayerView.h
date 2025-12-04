/**
 * ************************************************************************
 *
 * @file MainPlayerView.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-04
 * @version 0.1
 * @brief 主玩家视图组件（header-only）
 *
 * 该组件用于显示主玩家的信息卡片与操作区，布局说明：
 * - 整体为横向布局，左侧为玩家卡片（3），右侧为操作区（7）
 * - 左侧区域为CharacterView, 显示玩家角色信息
 * - 右侧区域为操作区
 

    - 提供方法
    - 提供方法修改玩家名称与状态显示
    - 提供方法修改身份、阵营、角色显示
 *
 * ************************************************************************
 */

#pragma once

#include <memory>
#include "src/client/ui/Layout.h"
#include "src/client/ui/Label.h"
#include "src/client/ui/Button.h"
#include "src/client/ui/ListArea.h"
#include "src/client/ui/Image.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
class MainPlayerView
{
public:
    // 返回一个构建好的主玩家视图 widget
    static std::shared_ptr<ui::Widget> create()
    {
        auto root = std::make_shared<ui::HBoxLayout>();
        root->setSpacing(10);

        // 左侧卡片区域（3）
        root->addWidget(createPlayerCard(), 3);

        // 右侧操作与信息区（7）
        root->addWidget(createActionArea(), 7);

        return root;
    }

private:
    static std::shared_ptr<ui::Widget> createPlayerCard()
    {
        auto card = std::make_shared<ui::VBoxLayout>();
        card->setBackgroundEnabled(true);
        card->setBackgroundColor(ImVec4(0.12F, 0.14F, 0.17F, 0.9F));
        card->setMargins(8, 8, 8, 8);
        card->setSpacing(6);
        card->setFixedSize(320, 180); // 宽大于高的长方形

        // 卡面（可替换为图片）
        auto face = std::make_shared<ui::Image>();
        face->setFixedSize(300, 120);
        face->setBackgroundEnabled(true);
        face->setBackgroundColor(ImVec4(0.16F, 0.18F, 0.22F, 0.95F));

        // 左上角的小竖向 ListArea（身份/阵营/角色）
        auto infoList = std::make_shared<ui::ListArea>(ui::ListDirection::VERTICAL);
        infoList->setScrollbarEnabled(false);
        infoList->setItemSpacing(2.0F);
        infoList->setMargins(4.0F);
        infoList->addWidget(std::make_shared<ui::Label>("身份: 主公"));
        infoList->addWidget(std::make_shared<ui::Label>("阵营: 蜀"));
        infoList->addWidget(std::make_shared<ui::Label>("角色: 刘备"));

        // 将卡面和 infoList 放在同一行（infoList 置于左上）
        auto topRow = std::make_shared<ui::HBoxLayout>();
        topRow->setSpacing(6);
        topRow->addWidget(infoList, 0, ui::Alignment::TOP | ui::Alignment::LEFT);
        topRow->addWidget(face, 1);

        card->addWidget(topRow);

        // 右侧横向的玩家名称与状态（放在卡片下方，靠右）
        auto nameRow = std::make_shared<ui::HBoxLayout>();
        nameRow->setSpacing(8);
        nameRow->addStretch(1);
        auto nameLabel = std::make_shared<ui::Label>("玩家: Anakin");
        auto statusLabel = std::make_shared<ui::Label>("状态: 在线");
        nameRow->addWidget(nameLabel);
        nameRow->addWidget(statusLabel);

        card->addWidget(nameRow);
        card->addStretch(1);
        return card;
    }

    static std::shared_ptr<ui::Widget> createActionArea()
    {
        auto right = std::make_shared<ui::VBoxLayout>();
        right->setBackgroundEnabled(true);
        right->setBackgroundColor(ImVec4(0.10F, 0.13F, 0.16F, 0.9F));
        right->setMargins(8, 8, 8, 8);
        right->setSpacing(8);

        // 顶部：玩家名称大标题
        auto title = std::make_shared<ui::Label>("Anakin2759");
        right->addWidget(title);

        // 中部：状态信息（血量/护甲/手牌等）
        auto statusArea = std::make_shared<ui::HBoxLayout>();
        statusArea->setSpacing(10);
        statusArea->addWidget(std::make_shared<ui::Label>("HP: 4/4 ♥♥♥♥"));
        statusArea->addWidget(std::make_shared<ui::Label>("手牌: 5"));
        statusArea->addWidget(std::make_shared<ui::Label>("距离: 1"));
        right->addWidget(statusArea);

        // 操作区：按钮
        auto ops = std::make_shared<ui::HBoxLayout>();
        ops->setSpacing(6);
        ops->addWidget(std::make_shared<ui::Button>("使用卡牌"));
        ops->addWidget(std::make_shared<ui::Button>("使用技能"));
        ops->addWidget(std::make_shared<ui::Button>("结束回合"));
        right->addWidget(ops);

        // 扩展：技能 / 装备 列表
        auto lower = std::make_shared<ui::HBoxLayout>();
        lower->setSpacing(8);
        auto skills = std::make_shared<ui::ListArea>(ui::ListDirection::VERTICAL);
        skills->addWidget(std::make_shared<ui::Button>("仁德"));
        skills->addWidget(std::make_shared<ui::Button>("激将"));
        lower->addWidget(skills, 1);

        auto equipment = std::make_shared<ui::VBoxLayout>();
        equipment->addWidget(std::make_shared<ui::Label>("武器: 青釭剑"));
        equipment->addWidget(std::make_shared<ui::Label>("防具: 八卦阵"));
        lower->addWidget(equipment, 1);

        right->addWidget(lower, 1);
        right->addStretch(1);
        return right;
    }
};

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
