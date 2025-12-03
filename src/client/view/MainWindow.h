/**
 * ************************************************************************
 *
 * @file MainWindow.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-03
 * @version 0.1
 * @brief 继承ui::Application，定义主窗口和UI布局
  模拟 Qt 的主窗口类 QMainWindow
  定义主窗口的左侧面板、中央处理区等UI布局
  基于ui::Application实现应用框架
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <memory>
#include "src/client/ui/Application.h"
#include "src/client/ui/Label.h"
#include "src/client/ui/Button.h"
#include "src/client/ui/ListArea.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
class MainWindow : public ui::Application
{
public:
    using ui::Application::Application;

protected:
    void setupUI() override
    {
        auto mainLayout = std::make_shared<ui::VBoxLayout>();
        mainLayout->setSpacing(8);
        //    mainLayout->setMargins(10, 10, 10, 10);

        mainLayout->addWidget(createTopOpponentsArea(), 2);
        mainLayout->addWidget(createMiddleGameArea(), 4);
        mainLayout->addWidget(createBottomPlayerArea(), 4);

        setRootLayout(mainLayout);
    }

private:
    // 创建顶部对手区域
    static std::shared_ptr<ui::Widget> createTopOpponentsArea()
    {
        auto topLayout = std::make_shared<ui::HBoxLayout>();
        topLayout->setBackgroundEnabled(true);
        topLayout->setBackgroundColor(ImVec4(0.12F, 0.15F, 0.18F, 0.75F));
        topLayout->setSpacing(20);

        for (int i = 0; i < 3; ++i)
        {
            topLayout->addWidget(createOpponentCard(i + 2));
        }
        return topLayout;
    }

    // 创建单个对手卡片
    static std::shared_ptr<ui::Widget> createOpponentCard(int playerNum)
    {
        auto card = std::make_shared<ui::VBoxLayout>();
        card->setBackgroundEnabled(true);
        card->setBackgroundColor(ImVec4(0.15F, 0.18F, 0.22F, 0.85F));
        card->setFixedSize(140, 180);
        card->setMargins(10, 10, 10, 10);

        card->addWidget(std::make_shared<ui::Label>("玩家" + std::to_string(playerNum)));
        card->addWidget(std::make_shared<ui::Label>("[红桃] HP: 4/4"));
        card->addWidget(std::make_shared<ui::Label>("手牌: 3"));
        card->addStretch(1);
        return card;
    }

    // 创建中间游戏区域
    static std::shared_ptr<ui::Widget> createMiddleGameArea()
    {
        auto middleLayout = std::make_shared<ui::HBoxLayout>();
        middleLayout->setSpacing(10);

        middleLayout->addWidget(createLeftSidePanel());
        middleLayout->addWidget(createCenterPanel(), 1);
        middleLayout->addWidget(createRightSidePanel());
        return middleLayout;
    }

    // 创建左侧面板（牌堆和弃牌堆）
    static std::shared_ptr<ui::Widget> createLeftSidePanel()
    {
        auto leftPanel = std::make_shared<ui::VBoxLayout>();
        leftPanel->setBackgroundEnabled(true);
        leftPanel->setBackgroundColor(ImVec4(0.14F, 0.16F, 0.20F, 0.80F));
        leftPanel->setFixedSize(160, 0);
        leftPanel->setMargins(10, 10, 10, 10);
        leftPanel->setSpacing(10);

        leftPanel->addWidget(createDeckArea());
        leftPanel->addWidget(createDiscardArea());
        leftPanel->addStretch(1);
        return leftPanel;
    }

    static std::shared_ptr<ui::Widget> createDeckArea()
    {
        auto deck = std::make_shared<ui::VBoxLayout>();
        deck->setBackgroundEnabled(true);
        deck->setBackgroundColor(ImVec4(0.10F, 0.12F, 0.16F, 0.9F));
        deck->setMargins(5, 5, 5, 5);
        deck->addWidget(std::make_shared<ui::Label>("牌堆"));
        deck->addWidget(std::make_shared<ui::Label>("剩余: 58"));
        return deck;
    }

    static std::shared_ptr<ui::Widget> createDiscardArea()
    {
        auto discard = std::make_shared<ui::VBoxLayout>();
        discard->setBackgroundEnabled(true);
        discard->setBackgroundColor(ImVec4(0.10F, 0.12F, 0.16F, 0.9F));
        discard->setMargins(5, 5, 5, 5);
        discard->addWidget(std::make_shared<ui::Label>("弃牌堆"));
        discard->addWidget(std::make_shared<ui::Label>("数量: 12"));
        return discard;
    }

    // 创建中央面板（处理区）
    static std::shared_ptr<ui::Widget> createCenterPanel()
    {
        auto center = std::make_shared<ui::VBoxLayout>();
        center->setBackgroundEnabled(true);
        center->setBackgroundColor(ImVec4(0.12F, 0.14F, 0.18F, 0.55F));
        center->setSpacing(8);
        center->setMargins(15, 15, 15, 15);

        center->addWidget(createJudgeArea(), 1);
        center->addWidget(createBattleLogArea(), 3);
        return center;
    }

    static std::shared_ptr<ui::Widget> createJudgeArea()
    {
        auto judge = std::make_shared<ui::VBoxLayout>();
        judge->setBackgroundEnabled(true);
        judge->setBackgroundColor(ImVec4(0.16F, 0.18F, 0.22F, 0.65F));
        judge->addStretch(1);
        return judge;
    }

    static std::shared_ptr<ui::Widget> createBattleLogArea()
    {
        auto battleLog = std::make_shared<ui::VBoxLayout>();
        return battleLog;
    }

    // 创建右侧面板（战斗记录）
    static std::shared_ptr<ui::Widget> createRightSidePanel()
    {
        auto battleLog = std::make_shared<ui::VBoxLayout>();
        battleLog->setBackgroundEnabled(true);
        battleLog->setBackgroundColor(ImVec4(0.10F, 0.12F, 0.15F, 0.75F));
        battleLog->setMargins(5, 5, 5, 5);
        battleLog->addWidget(std::make_shared<ui::Label>("战斗记录:"));
        battleLog->addWidget(std::make_shared<ui::Label>("刘备对张飞使用【杀】"));
        battleLog->addWidget(std::make_shared<ui::Label>("张飞使用【闪】"));
        battleLog->addStretch(1);
        return battleLog;
    }

    // 创建底部玩家区域
    static std::shared_ptr<ui::Widget> createBottomPlayerArea()
    {
        auto bottomArea = std::make_shared<ui::VBoxLayout>();
        bottomArea->setBackgroundEnabled(true);
        bottomArea->setBackgroundColor(ImVec4(0.13F, 0.16F, 0.20F, 0.82F));
        bottomArea->setSpacing(8);
        bottomArea->setMargins(10, 10, 10, 10);

        // 操作区在顶部
        bottomArea->addWidget(createActionButtonsArea());

        // 下方：角色区（左侧3）+ 手牌区（右侧7）
        auto bottomContentLayout = std::make_shared<ui::HBoxLayout>();
        bottomContentLayout->setSpacing(10);
        bottomContentLayout->addWidget(createPlayerCharacterArea(), 3);
        bottomContentLayout->addWidget(createHandCardsArea(), 7);

        bottomArea->addWidget(bottomContentLayout, 1);
        return bottomArea;
    }

    // 创建操作按钮区域（顶部）
    static std::shared_ptr<ui::Widget> createActionButtonsArea()
    {
        auto actionArea = std::make_shared<ui::HBoxLayout>();
        actionArea->setBackgroundEnabled(true);
        actionArea->setBackgroundColor(ImVec4(0.16F, 0.19F, 0.23F, 0.88F));
        actionArea->setMargins(8, 8, 8, 8);
        actionArea->setSpacing(10);

        actionArea->addStretch(1);
        actionArea->addWidget(std::make_shared<ui::Button>("出牌"));
        actionArea->addWidget(std::make_shared<ui::Button>("结束回合"));
        actionArea->addWidget(std::make_shared<ui::Button>("取消"));
        actionArea->addStretch(1);
        return actionArea;
    }

    // 创建玩家角色区域（左侧，包含角色信息、装备、技能）
    static std::shared_ptr<ui::Widget> createPlayerCharacterArea()
    {
        auto characterArea = std::make_shared<ui::VBoxLayout>();
        characterArea->setBackgroundEnabled(true);
        characterArea->setBackgroundColor(ImVec4(0.14F, 0.17F, 0.21F, 0.90F));
        characterArea->setMargins(10, 10, 10, 10);
        characterArea->setSpacing(8);

        // 角色基本信息
        characterArea->addWidget(createPlayerBasicInfo());

        // 装备区
        auto equipmentLabel = std::make_shared<ui::Label>("装备:");
        characterArea->addWidget(equipmentLabel);
        characterArea->addWidget(createPlayerEquipment());

        // 技能区
        auto skillLabel = std::make_shared<ui::Label>("技能:");
        characterArea->addWidget(skillLabel);
        characterArea->addWidget(createPlayerSkills());

        characterArea->addStretch(1);
        return characterArea;
    }

    static std::shared_ptr<ui::Widget> createPlayerBasicInfo()
    {
        auto info = std::make_shared<ui::VBoxLayout>();
        info->setSpacing(4);
        info->addWidget(std::make_shared<ui::Label>("【刘备】 主公"));
        info->addWidget(std::make_shared<ui::Label>("HP: 4/4 ♥♥♥♥"));
        return info;
    }

    static std::shared_ptr<ui::Widget> createPlayerEquipment()
    {
        auto equipment = std::make_shared<ui::VBoxLayout>();
        equipment->setSpacing(4);
        equipment->addWidget(std::make_shared<ui::Button>("武器: 青釭剑"));
        equipment->addWidget(std::make_shared<ui::Button>("防具: 八卦阵"));
        equipment->addWidget(std::make_shared<ui::Button>("坐骑: 的卢"));
        return equipment;
    }

    static std::shared_ptr<ui::Widget> createPlayerSkills()
    {
        auto skills = std::make_shared<ui::ListArea>(ui::ListDirection::VERTICAL);
        skills->setItemSpacing(4.0F);
        skills->addWidget(std::make_shared<ui::Button>("仁德"));
        skills->addWidget(std::make_shared<ui::Button>("激将"));
        return skills;
    }

    // 创建手牌区域（右侧）
    static std::shared_ptr<ui::Widget> createHandCardsArea()
    {
        auto handCards = std::make_shared<ui::VBoxLayout>();
        handCards->setBackgroundEnabled(true);
        handCards->setBackgroundColor(ImVec4(0.11F, 0.14F, 0.17F, 0.85F));
        handCards->setMargins(10, 10, 10, 10);
        handCards->setSpacing(8);

        auto label = std::make_shared<ui::Label>("手牌 (5张)");
        handCards->addWidget(label);
        handCards->addWidget(createCardsListArea(), 1);
        return handCards;
    }

    static std::shared_ptr<ui::Widget> createCardsListArea()
    {
        constexpr int CARD_WIDTH = 95;
        constexpr int CARD_HEIGHT = 135;

        auto cardsList = std::make_shared<ui::ListArea>(ui::ListDirection::HORIZONTAL);
        cardsList->setAlignment(ui::ListAlignment::START); // 左对齐
        cardsList->setItemSpacing(5.0F);                   // 设置卡牌间距
        cardsList->setScrollbarEnabled(true);              // 启用滚动条
        cardsList->setMargins(5.0F);                       // 设置边距

        // 创建卡牌按钮
        auto card1 = std::make_shared<ui::Button>("♠杀");
        card1->setFixedSize(CARD_WIDTH, CARD_HEIGHT);
        auto card2 = std::make_shared<ui::Button>("♥桃");
        card2->setFixedSize(CARD_WIDTH, CARD_HEIGHT);
        auto card3 = std::make_shared<ui::Button>("♦闪");
        card3->setFixedSize(CARD_WIDTH, CARD_HEIGHT);
        auto card4 = std::make_shared<ui::Button>("♣决斗");
        card4->setFixedSize(CARD_WIDTH, CARD_HEIGHT);
        auto card5 = std::make_shared<ui::Button>("♠无懈");
        card5->setFixedSize(CARD_WIDTH, CARD_HEIGHT);
        auto card6 = std::make_shared<ui::Button>("♠无懈");
        card6->setFixedSize(CARD_WIDTH, CARD_HEIGHT);
        auto card7 = std::make_shared<ui::Button>("♠无懈");
        card7->setFixedSize(CARD_WIDTH, CARD_HEIGHT);
        auto card8 = std::make_shared<ui::Button>("♠无懈");
        card8->setFixedSize(CARD_WIDTH, CARD_HEIGHT);

        // 添加到ListArea
        cardsList->addWidget(card1);
        cardsList->addWidget(card2);
        cardsList->addWidget(card3);
        cardsList->addWidget(card4);
        cardsList->addWidget(card5);
        cardsList->addWidget(card6);
        cardsList->addWidget(card7);
        cardsList->addWidget(card8);

        return cardsList;
    }
};
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)