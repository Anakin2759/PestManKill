/**
 * ************************************************************************
 *
 * @file Application.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-11-27
 * @version 0.1
 * @brief 主应用类定义
    IMGUI+SDL3+SDL_Renderer3实现的UI应用框架
    模拟 Qt 的应用类 QApplication
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */
#pragma once

#include "Layout.h"
#include "Label.h"
#include "Button.h"
#include <entt/entt.hpp>
#include <array>
#include <stdexcept>
#include <iostream>
// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
namespace ui
{
// 主应用类
class Application
{
public:
    constexpr static int DEFAULT_WINDOW_WIDTH = 1200;
    constexpr static int DEFAULT_WINDOW_HEIGHT = 800;
    constexpr static int DEFAULT_SPACING = 10;
    constexpr static int DEFAULT_MARGINS = 10;
    Application(const char* title, int width, int height)
    {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
        {
            std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
            throw std::runtime_error(SDL_GetError());
        }

        m_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_RESIZABLE);
        if (m_window == nullptr)
        {
            std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
            throw std::runtime_error(SDL_GetError());
        }
        std::cerr << "Window created: " << title << " (" << width << "x" << height << ")" << std::endl;
        m_renderer = SDL_CreateRenderer(m_window, nullptr);
        if (m_renderer == nullptr)
        {
            throw std::runtime_error(SDL_GetError());
        }

        initImGui();
        setupUI();
    }

    virtual ~Application()
    {
        shutdownImGui();
        if (m_renderer != nullptr)
        {
            SDL_DestroyRenderer(m_renderer);
        }
        if (m_window != nullptr)
        {
            SDL_DestroyWindow(m_window);
        }
        SDL_Quit();
    }

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    void run()
    {
        m_running = true;
        while (m_running)
        {
            processEvents();
            render();
        }
    }

    void stop() { m_running = false; }

protected:
    virtual void setupUI()
    {
        auto mainLayout = std::make_shared<VBoxLayout>();
        mainLayout->setBackgroundEnabled(true);
        mainLayout->setBackgroundColor(ImVec4(0.08F, 0.10F, 0.13F, 1.0F));
        mainLayout->setSpacing(8);
        mainLayout->setMargins(10, 10, 10, 10);

        mainLayout->addWidget(createTopOpponentsArea(), 2);
        mainLayout->addWidget(createMiddleGameArea(), 4);
        mainLayout->addWidget(createBottomPlayerArea(), 4);

        m_rootLayout = mainLayout;
    }

    // 创建顶部对手区域
    static std::shared_ptr<Widget> createTopOpponentsArea()
    {
        auto topLayout = std::make_shared<HBoxLayout>();
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
    static std::shared_ptr<Widget> createOpponentCard(int playerNum)
    {
        auto card = std::make_shared<VBoxLayout>();
        card->setBackgroundEnabled(true);
        card->setBackgroundColor(ImVec4(0.15F, 0.18F, 0.22F, 0.85F));
        card->setFixedSize(140, 180);
        card->setMargins(10, 10, 10, 10);

        card->addWidget(std::make_shared<Label>("玩家" + std::to_string(playerNum)));
        card->addWidget(std::make_shared<Label>("[红桃] HP: 4/4"));
        card->addWidget(std::make_shared<Label>("手牌: 3"));
        card->addStretch(1);
        return card;
    }

    // 创建中间游戏区域
    static std::shared_ptr<Widget> createMiddleGameArea()
    {
        auto middleLayout = std::make_shared<HBoxLayout>();
        middleLayout->setSpacing(10);

        middleLayout->addWidget(createLeftSidePanel());
        middleLayout->addWidget(createCenterPanel(), 1);
        middleLayout->addWidget(createRightSidePanel());
        return middleLayout;
    }

    // 创建左侧面板（牌堆和弃牌堆）
    static std::shared_ptr<Widget> createLeftSidePanel()
    {
        auto leftPanel = std::make_shared<VBoxLayout>();
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

    static std::shared_ptr<Widget> createDeckArea()
    {
        auto deck = std::make_shared<VBoxLayout>();
        deck->setBackgroundEnabled(true);
        deck->setBackgroundColor(ImVec4(0.10F, 0.12F, 0.16F, 0.9F));
        deck->setMargins(5, 5, 5, 5);
        deck->addWidget(std::make_shared<Label>("牌堆"));
        deck->addWidget(std::make_shared<Label>("剩余: 58"));
        return deck;
    }

    static std::shared_ptr<Widget> createDiscardArea()
    {
        auto discard = std::make_shared<VBoxLayout>();
        discard->setBackgroundEnabled(true);
        discard->setBackgroundColor(ImVec4(0.10F, 0.12F, 0.16F, 0.9F));
        discard->setMargins(5, 5, 5, 5);
        discard->addWidget(std::make_shared<Label>("弃牌堆"));
        discard->addWidget(std::make_shared<Label>("数量: 12"));
        return discard;
    }

    // 创建中央面板（战斗和判定区）
    static std::shared_ptr<Widget> createCenterPanel()
    {
        auto center = std::make_shared<VBoxLayout>();
        center->setBackgroundEnabled(true);
        center->setBackgroundColor(ImVec4(0.12F, 0.14F, 0.18F, 0.55F));
        center->setSpacing(8);
        center->setMargins(15, 15, 15, 15);

        center->addWidget(createJudgeArea(), 1);
        center->addWidget(createBattleLogArea(), 3);
        return center;
    }

    static std::shared_ptr<Widget> createJudgeArea()
    {
        auto judge = std::make_shared<VBoxLayout>();
        judge->setBackgroundEnabled(true);
        judge->setBackgroundColor(ImVec4(0.16F, 0.18F, 0.22F, 0.65F));
        judge->addWidget(std::make_shared<Label>("判定区"));
        judge->addWidget(std::make_shared<Label>("当前回合: 刘备"));
        judge->addStretch(1);
        return judge;
    }

    static std::shared_ptr<Widget> createBattleLogArea()
    {
        auto battleLog = std::make_shared<VBoxLayout>();

        return battleLog;
    }

    // 创建右侧面板（装备区）
    static std::shared_ptr<Widget> createRightSidePanel()
    {
        auto battleLog = std::make_shared<VBoxLayout>();
        battleLog->setBackgroundEnabled(true);
        battleLog->setBackgroundColor(ImVec4(0.10F, 0.12F, 0.15F, 0.75F));
        battleLog->setMargins(5, 5, 5, 5);
        battleLog->addWidget(std::make_shared<Label>("战斗记录:"));
        battleLog->addWidget(std::make_shared<Label>("刘备对张飞使用【杀】"));
        battleLog->addWidget(std::make_shared<Label>("张飞使用【闪】"));
        battleLog->addStretch(1);
        return battleLog;
    }

    // 创建底部玩家区域
    static std::shared_ptr<Widget> createBottomPlayerArea()
    {
        auto bottomArea = std::make_shared<VBoxLayout>();
        bottomArea->setBackgroundEnabled(true);
        bottomArea->setBackgroundColor(ImVec4(0.13F, 0.16F, 0.20F, 0.82F));
        bottomArea->setSpacing(8);
        bottomArea->setMargins(10, 10, 10, 10);

        // 操作区在顶部
        bottomArea->addWidget(createActionButtonsArea());

        // 下方：角色区（左侧3）+ 手牌区（右侧7）
        auto bottomContentLayout = std::make_shared<HBoxLayout>();
        bottomContentLayout->setSpacing(10);
        bottomContentLayout->addWidget(createPlayerCharacterArea(), 3);
        bottomContentLayout->addWidget(createHandCardsArea(), 7);

        bottomArea->addWidget(bottomContentLayout, 1);
        return bottomArea;
    }

    // 创建操作按钮区域（顶部）
    static std::shared_ptr<Widget> createActionButtonsArea()
    {
        auto actionArea = std::make_shared<HBoxLayout>();
        actionArea->setBackgroundEnabled(true);
        actionArea->setBackgroundColor(ImVec4(0.16F, 0.19F, 0.23F, 0.88F));
        actionArea->setMargins(8, 8, 8, 8);
        actionArea->setSpacing(10);

        actionArea->addStretch(1);
        actionArea->addWidget(std::make_shared<Button>("出牌"));
        actionArea->addWidget(std::make_shared<Button>("结束回合"));
        actionArea->addWidget(std::make_shared<Button>("取消"));
        actionArea->addStretch(1);
        return actionArea;
    }

    // 创建玩家角色区域（左侧，包含角色信息、装备、技能）
    static std::shared_ptr<Widget> createPlayerCharacterArea()
    {
        auto characterArea = std::make_shared<VBoxLayout>();
        characterArea->setBackgroundEnabled(true);
        characterArea->setBackgroundColor(ImVec4(0.14F, 0.17F, 0.21F, 0.90F));
        characterArea->setMargins(10, 10, 10, 10);
        characterArea->setSpacing(8);

        // 角色基本信息
        characterArea->addWidget(createPlayerBasicInfo());

        // 装备区
        auto equipmentLabel = std::make_shared<Label>("装备:");
        characterArea->addWidget(equipmentLabel);
        characterArea->addWidget(createPlayerEquipment());

        // 技能区
        auto skillLabel = std::make_shared<Label>("技能:");
        characterArea->addWidget(skillLabel);
        characterArea->addWidget(createPlayerSkills());

        characterArea->addStretch(1);
        return characterArea;
    }

    static std::shared_ptr<Widget> createPlayerBasicInfo()
    {
        auto info = std::make_shared<VBoxLayout>();
        info->setSpacing(4);
        info->addWidget(std::make_shared<Label>("【刘备】 主公"));
        info->addWidget(std::make_shared<Label>("HP: 4/4 ♥♥♥♥"));
        return info;
    }

    static std::shared_ptr<Widget> createPlayerEquipment()
    {
        auto equipment = std::make_shared<VBoxLayout>();
        equipment->setSpacing(4);
        equipment->addWidget(std::make_shared<Button>("武器: 青釭剑"));
        equipment->addWidget(std::make_shared<Button>("防具: 八卦阵"));
        equipment->addWidget(std::make_shared<Button>("坐骑: 的卢"));
        return equipment;
    }

    static std::shared_ptr<Widget> createPlayerSkills()
    {
        auto skills = std::make_shared<VBoxLayout>();
        skills->setSpacing(4);
        skills->addWidget(std::make_shared<Button>("仁德"));
        skills->addWidget(std::make_shared<Button>("激将"));
        return skills;
    }

    // 创建手牌区域（右侧）
    static std::shared_ptr<Widget> createHandCardsArea()
    {
        auto handCards = std::make_shared<VBoxLayout>();
        handCards->setBackgroundEnabled(true);
        handCards->setBackgroundColor(ImVec4(0.11F, 0.14F, 0.17F, 0.85F));
        handCards->setMargins(10, 10, 10, 10);
        handCards->setSpacing(8);

        auto label = std::make_shared<Label>("手牌 (5张)");
        handCards->addWidget(label);
        handCards->addWidget(createCardsRow(), 1);
        return handCards;
    }

    static std::shared_ptr<Widget> createCardsRow()
    {
        constexpr int CARD_WIDTH = 95;
        constexpr int CARD_HEIGHT = 135;

        auto cardsRow = std::make_shared<HBoxLayout>();
        cardsRow->setSpacing(10);

        auto card1 = std::make_shared<Button>("♠杀");
        card1->setFixedSize(CARD_WIDTH, CARD_HEIGHT);
        auto card2 = std::make_shared<Button>("♥桃");
        card2->setFixedSize(CARD_WIDTH, CARD_HEIGHT);
        auto card3 = std::make_shared<Button>("♦闪");
        card3->setFixedSize(CARD_WIDTH, CARD_HEIGHT);
        auto card4 = std::make_shared<Button>("♣决斗");
        card4->setFixedSize(CARD_WIDTH, CARD_HEIGHT);
        auto card5 = std::make_shared<Button>("♠无懈");
        card5->setFixedSize(CARD_WIDTH, CARD_HEIGHT);

        cardsRow->addWidget(card1);
        cardsRow->addWidget(card2);
        cardsRow->addWidget(card3);
        cardsRow->addWidget(card4);
        cardsRow->addWidget(card5);
        cardsRow->addStretch(1);
        return cardsRow;
    }

    virtual void onGui()
    {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

        ImGui::Begin("MainWindow",
                     nullptr,
                     static_cast<ImGuiWindowFlags>(static_cast<unsigned int>(ImGuiWindowFlags_NoTitleBar) |
                                                   static_cast<unsigned int>(ImGuiWindowFlags_NoResize) |
                                                   static_cast<unsigned int>(ImGuiWindowFlags_NoMove) |
                                                   static_cast<unsigned int>(ImGuiWindowFlags_NoScrollbar) |
                                                   static_cast<unsigned int>(ImGuiWindowFlags_NoScrollWithMouse) |
                                                   static_cast<unsigned int>(ImGuiWindowFlags_NoCollapse)));

        if (m_rootLayout)
        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            m_rootLayout->render(ImVec2(0, 0), avail);
        }

        ImGui::End();
    }

private:
    SDL_Window* m_window = nullptr;
    SDL_Renderer* m_renderer = nullptr;
    bool m_running = true;
    std::shared_ptr<Layout> m_rootLayout;

    void processEvents()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT)
            {
                m_running = false;
            }
        }
    }

    void render()
    {
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        onGui();

        ImGui::Render();

        SDL_SetRenderDrawColor(m_renderer, 20, 25, 33, 255);
        SDL_RenderClear(m_renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_renderer);
        SDL_RenderPresent(m_renderer);
    }

    void initImGui()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGuiIO& imGuiIo = ImGui::GetIO();
        ImFontConfig fontCfg;
        fontCfg.OversampleH = 3;
        fontCfg.OversampleV = 1;
        fontCfg.PixelSnapH = true;

        // 加载中文字体，并合并常用符号
        static constexpr std::array<ImWchar, 12> FONT_RANGES = {
            0x0020,
            0x00FF, // 基本拉丁字母
            0x2000,
            0x206F, // 常规标点
            0x2600,
            0x26FF, // 杂项符号（包含扑克牌花色）
            0x4E00,
            0x9FFF, // CJK统一汉字
            0,
        };

        imGuiIo.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/msyh.ttc",
                                          // NOLINTNEXTLINE
                                          18.0F,
                                          &fontCfg,
                                          FONT_RANGES.data());

        ImGui_ImplSDL3_InitForSDLRenderer(m_window, m_renderer);
        ImGui_ImplSDLRenderer3_Init(m_renderer);
    }

    static void shutdownImGui()
    {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
};
} // namespace ui
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)