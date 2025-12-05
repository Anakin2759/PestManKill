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

#include "CharacterView.h"
#include "DeckView.h"
#include "EquipmentArea.h"
#include "HandCardView.h"
#include "OperationArea.h"
#include "OtherPlayerView.h"
#include "ProcessingArea.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
class MainWindow : public ui::Application
{
public:
    using ui::Application::Application;

    // ==================== 公共接口 ====================

    // 玩家角色相关
    void updatePlayerCharacter(const std::string& name, const std::string& identity, const std::string& faction)
    {
        if (m_playerCharacter)
        {
            m_playerCharacter->setCharacterName(name);
            m_playerCharacter->setIdentity(identity);
            m_playerCharacter->setFaction(faction);
        }
    }

    void updatePlayerHp(int current, int max)
    {
        if (m_playerCharacter)
        {
            m_playerCharacter->setHp(current, max);
        }
    }

    void addPlayerSkill(const std::string& name, const std::string& description)
    {
        if (m_playerCharacter)
        {
            m_playerCharacter->addSkill({name, description});
        }
    }

    // 装备相关
    void equipPlayerWeapon(const std::string& name, const std::string& iconPath = "", int range = 1)
    {
        if (m_equipmentArea)
        {
            m_equipmentArea->equipWeapon(name, iconPath);
        }
    }

    void equipPlayerArmor(const std::string& name, const std::string& iconPath = "")
    {
        if (m_equipmentArea)
        {
            m_equipmentArea->equipArmor(name, iconPath);
        }
    }

    void unequipPlayerWeapon()
    {
        if (m_equipmentArea)
        {
            m_equipmentArea->unequipWeapon();
        }
    }

    // 手牌相关
    void addHandCard(std::shared_ptr<HandCardView> card)
    {
        if (m_operationArea)
        {
            m_operationArea->addHandCard(card);
        }
    }

    void clearHandCards()
    {
        if (m_operationArea)
        {
            m_operationArea->clearHandCards();
        }
    }

    // 处理区相关
    void addCardToProcessing(std::shared_ptr<HandCardView> card)
    {
        if (m_processingArea)
        {
            m_processingArea->addCard(card);
        }
    }

    void clearProcessingArea()
    {
        if (m_processingArea)
        {
            m_processingArea->clear();
        }
    }

    // 牌堆相关
    void updateDeckCount(int deckCount, int discardCount)
    {
        if (m_deckView)
        {
            m_deckView->setDeckCount(deckCount);
            m_deckView->setDiscardCount(discardCount);
        }
    }

    // 对手相关
    void updateOpponent(size_t index, const std::string& name, int currentHp, int maxHp, int handCount)
    {
        if (index < m_opponentViews.size())
        {
            m_opponentViews[index]->setPlayerName(name);
            m_opponentViews[index]->setHp(currentHp, maxHp);
            m_opponentViews[index]->setHandCardCount(handCount);
        }
    }

private:
    // 组件引用（用于动态更新）
    std::shared_ptr<CharacterView> m_playerCharacter;
    std::shared_ptr<EquipmentArea> m_equipmentArea;
    std::shared_ptr<OperationArea> m_operationArea;
    std::shared_ptr<ProcessingArea> m_processingArea;
    std::shared_ptr<DeckView> m_deckView;
    std::vector<std::shared_ptr<OtherPlayerView>> m_opponentViews;

protected:
    void setupUI() override
    {
        auto mainLayout = std::make_shared<ui::VBoxLayout>();
        mainLayout->setSpacing(10);
        mainLayout->setMargins(5, 5, 5, 5);

        // 顶部：对手区域
        mainLayout->addWidget(createTopOpponentsArea(), 2);

        // 中间：游戏区域（左侧牌堆 + 中央处理区 + 右侧战斗记录）
        mainLayout->addWidget(createMiddleGameArea(), 4);

        // 底部：玩家区域（角色信息 + 装备 + 操作区）
        mainLayout->addWidget(createBottomPlayerArea(), 4);

        setRootLayout(mainLayout);

        // 初始化测试数据
        initializeTestData();
    }

private:
    // ==================== 顶部对手区域 ====================
    std::shared_ptr<ui::Widget> createTopOpponentsArea()
    {
        auto topLayout = std::make_shared<ui::HBoxLayout>();
        topLayout->setBackgroundEnabled(true);
        topLayout->setBackgroundColor(ImVec4(0.12F, 0.15F, 0.18F, 0.75F));
        topLayout->setSpacing(15);
        topLayout->setMargins(10, 10, 10, 10);

        // 创建3个对手视图
        for (int i = 0; i < 3; ++i)
        {
            auto opponent = std::make_shared<OtherPlayerView>();
            opponent->setPlayerName("玩家" + std::to_string(i + 2));
            opponent->setHp(4, 4);
            opponent->setHandCardCount(3);
            opponent->setIdentity("忠臣");
            opponent->setOnlineStatus(true);

            m_opponentViews.push_back(opponent);
            topLayout->addWidget(opponent);
        }

        return topLayout;
    }

    // ==================== 中间游戏区域 ====================
    std::shared_ptr<ui::Widget> createMiddleGameArea()
    {
        auto middleLayout = std::make_shared<ui::HBoxLayout>();
        middleLayout->setSpacing(10);

        // 左侧：牌堆和弃牌堆
        middleLayout->addWidget(createLeftSidePanel());

        // 中央：处理区
        middleLayout->addWidget(createCenterPanel(), 1);

        // 右侧：战斗记录
        middleLayout->addWidget(createRightSidePanel());

        return middleLayout;
    }

    std::shared_ptr<ui::Widget> createLeftSidePanel()
    {
        auto leftPanel = std::make_shared<ui::VBoxLayout>();
        leftPanel->setBackgroundEnabled(true);
        leftPanel->setBackgroundColor(ImVec4(0.14F, 0.16F, 0.20F, 0.80F));
        leftPanel->setFixedSize(180, 0);
        leftPanel->setMargins(10, 10, 10, 10);
        leftPanel->setSpacing(10);

        // 使用 DeckView 组件
        m_deckView = std::make_shared<DeckView>();
        leftPanel->addWidget(m_deckView);

        leftPanel->addStretch(1);
        return leftPanel;
    }

    std::shared_ptr<ui::Widget> createCenterPanel()
    {
        auto center = std::make_shared<ui::VBoxLayout>();
        center->setBackgroundEnabled(true);
        center->setBackgroundColor(ImVec4(0.12F, 0.14F, 0.18F, 0.55F));
        center->setMargins(15, 15, 15, 15);

        // 使用 ProcessingArea 组件
        m_processingArea = std::make_shared<ProcessingArea>();
        center->addWidget(m_processingArea, 1);

        return center;
    }

    std::shared_ptr<ui::Widget> createRightSidePanel()
    {
        auto battleLog = std::make_shared<ui::VBoxLayout>();
        battleLog->setBackgroundEnabled(true);
        battleLog->setBackgroundColor(ImVec4(0.10F, 0.12F, 0.15F, 0.75F));
        battleLog->setFixedSize(200, 0);
        battleLog->setMargins(10, 10, 10, 10);
        battleLog->setSpacing(5);

        auto title = std::make_shared<ui::Label>("战斗记录:");
        title->setTextColor(ImVec4(1.0F, 0.8F, 0.4F, 1.0F));
        battleLog->addWidget(title);

        // 战斗记录列表
        auto logList = std::make_shared<ui::ListArea>(ui::ListDirection::VERTICAL);
        logList->setScrollbarEnabled(true);
        logList->setItemSpacing(3.0F);

        auto log1 = std::make_shared<ui::Label>("刘备对张飞使用【杀】");
        log1->setTextColor(ImVec4(0.9F, 0.9F, 0.9F, 1.0F));
        auto log2 = std::make_shared<ui::Label>("张飞使用【闪】");
        log2->setTextColor(ImVec4(0.9F, 0.9F, 0.9F, 1.0F));

        logList->addWidget(log1);
        logList->addWidget(log2);

        battleLog->addWidget(logList, 1);
        return battleLog;
    }

    // ==================== 底部玩家区域 ====================
    std::shared_ptr<ui::Widget> createBottomPlayerArea()
    {
        auto bottomArea = std::make_shared<ui::HBoxLayout>();
        bottomArea->setBackgroundEnabled(true);
        bottomArea->setBackgroundColor(ImVec4(0.13F, 0.16F, 0.20F, 0.82F));
        bottomArea->setSpacing(10);
        bottomArea->setMargins(10, 10, 10, 10);

        // 左侧：角色信息 + 装备区
        bottomArea->addWidget(createPlayerInfoArea(), 3);

        // 右侧：操作区（按钮 + 手牌）
        bottomArea->addWidget(createPlayerOperationArea(), 7);

        return bottomArea;
    }

    std::shared_ptr<ui::Widget> createPlayerInfoArea()
    {
        auto infoArea = std::make_shared<ui::VBoxLayout>();
        infoArea->setSpacing(10);

        // 角色视图
        m_playerCharacter = std::make_shared<CharacterView>();
        infoArea->addWidget(m_playerCharacter, 3);

        // 装备区
        m_equipmentArea = std::make_shared<EquipmentArea>();
        infoArea->addWidget(m_equipmentArea, 2);

        return infoArea;
    }

    std::shared_ptr<ui::Widget> createPlayerOperationArea()
    {
        // 使用 OperationArea 组件
        m_operationArea = std::make_shared<OperationArea>();

        // 设置按钮回调
        m_operationArea->setOnUseCard(
            [this]()
            {
                // 处理使用卡牌逻辑
                utils::LOG_INFO("使用卡牌按钮被点击");
            });

        m_operationArea->setOnCancel(
            [this]()
            {
                // 处理取消逻辑
                utils::LOG_INFO("取消按钮被点击");
            });

        m_operationArea->setOnEndTurn(
            [this]()
            {
                // 处理结束回合逻辑
                utils::LOG_INFO("结束回合按钮被点击");
            });

        return m_operationArea;
    }

    // ==================== 初始化测试数据 ====================
    void initializeTestData()
    {
        // 初始化玩家角色
        if (m_playerCharacter)
        {
            m_playerCharacter->setCharacterName("刘备");
            m_playerCharacter->setIdentity("主公");
            m_playerCharacter->setFaction("蜀");
            m_playerCharacter->setHp(4, 4);

            // 添加技能
            m_playerCharacter->addSkill(
                {"仁德", "出牌阶段，你可以将任意张手牌交给其他角色，若你在此阶段给出的牌不少于两张，你回复1点体力。"});
            m_playerCharacter->addSkill({"激将",
                                         "主公技，当你需要使用或打出【杀】时，你可以令其他蜀势力角色选择是否打出一张【"
                                         "杀】（视为由你使用或打出）。"});
        }

        // 初始化装备
        if (m_equipmentArea)
        {
            m_equipmentArea->equipWeapon("青釭剑", "");
            m_equipmentArea->equipArmor("八卦阵", "");
        }

        // 初始化牌堆
        if (m_deckView)
        {
            m_deckView->setDeckCount(58);
            m_deckView->setDiscardCount(12);
        }

        // 初始化手牌
        if (m_operationArea)
        {
            for (int i = 0; i < 5; ++i)
            {
                auto card = std::make_shared<HandCardView>();
                std::vector<std::string> cardNames = {"杀", "闪", "桃", "决斗", "无懈可击"};
                std::vector<std::string> suits = {"♠", "♥", "♦", "♣", "♠"};

                card->setCardName(suits[i] + cardNames[i]);
                card->setCardSuit(suits[i]);
                card->setCardRank(std::to_string(i + 1));

                m_operationArea->addHandCard(card);
            }
        }
    }
};
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)