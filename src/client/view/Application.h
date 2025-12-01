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
#include "Image.h"
#include <entt/entt.hpp>
#include <iostream>
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
        // 创建主布局
        auto mainLayout = std::make_shared<VBoxLayout>();
        mainLayout->setBackgroundEnabled(true);
        // NOLINTBEGIN
        mainLayout->setBackgroundColor(ImVec4(0.15F, 0.12F, 0.10F, 1.0F)); // 木桌背景色
        mainLayout->setSpacing(5);
        mainLayout->setMargins(10, 10, 10, 10);
        // NOLINTEND

        // ========== 顶部区域：对手玩家 ==========
        auto topOpponentsLayout = std::make_shared<HBoxLayout>();
        topOpponentsLayout->setBackgroundEnabled(true);
        // NOLINTBEGIN
        topOpponentsLayout->setBackgroundColor(ImVec4(0.25F, 0.20F, 0.15F, 0.8F));
        topOpponentsLayout->setSpacing(15);
        // NOLINTEND

        // 添加3个对手
        for (int i = 0; i < 3; ++i)
        {
            auto opponentCard = std::make_shared<VBoxLayout>();
            opponentCard->setBackgroundEnabled(true);
            // NOLINTBEGIN
            opponentCard->setBackgroundColor(ImVec4(0.35F, 0.25F, 0.20F, 0.9F));
            opponentCard->setFixedSize(120, 160);
            opponentCard->setMargins(8, 8, 8, 8);
            // NOLINTEND

            opponentCard->addWidget(std::make_shared<Label>("玩家" + std::to_string(i + 2)));
            opponentCard->addWidget(std::make_shared<Label>("[红桃] HP: 4/4"));
            opponentCard->addWidget(std::make_shared<Label>("手牌: 3"));
            opponentCard->addStretch(1);

            topOpponentsLayout->addWidget(opponentCard);
        }

        // ========== 中间区域：游戏主区域 ==========
        auto middleLayout = std::make_shared<HBoxLayout>();
        // NOLINTNEXTLINE
        middleLayout->setSpacing(10);

        // 左侧：牌堆和弃牌堆
        auto leftSideLayout = std::make_shared<VBoxLayout>();
        leftSideLayout->setBackgroundEnabled(true);
        // NOLINTBEGIN
        leftSideLayout->setBackgroundColor(ImVec4(0.30F, 0.22F, 0.15F, 0.85F));
        leftSideLayout->setFixedSize(150, 0);
        leftSideLayout->setMargins(10, 10, 10, 10);
        leftSideLayout->setSpacing(10);
        // NOLINTEND

        auto deckArea = std::make_shared<VBoxLayout>();
        deckArea->setBackgroundEnabled(true);
        deckArea->setBackgroundColor(ImVec4(0.2F, 0.15F, 0.12F, 1.0F));
        deckArea->setMargins(5, 5, 5, 5);
        deckArea->addWidget(std::make_shared<Label>("牌堆"));
        deckArea->addWidget(std::make_shared<Label>("剩余: 58"));
        leftSideLayout->addWidget(deckArea);

        auto discardArea = std::make_shared<VBoxLayout>();
        discardArea->setBackgroundEnabled(true);
        discardArea->setBackgroundColor(ImVec4(0.2F, 0.15F, 0.12F, 1.0F));
        discardArea->setMargins(5, 5, 5, 5);
        discardArea->addWidget(std::make_shared<Label>("弃牌堆"));
        discardArea->addWidget(std::make_shared<Label>("数量: 12"));
        leftSideLayout->addWidget(discardArea);

        leftSideLayout->addStretch(1);

        // 中间：战斗和判定区
        auto centerArea = std::make_shared<VBoxLayout>();
        centerArea->setBackgroundEnabled(true);
        // NOLINTNEXTLINE
        centerArea->setBackgroundColor(ImVec4(0.20F, 0.25F, 0.20F, 0.6F));
        centerArea->setSpacing(8);
        // NOLINTNEXTLINE
        centerArea->setMargins(15, 15, 15, 15);

        // 判定区
        auto judgeArea = std::make_shared<VBoxLayout>();
        judgeArea->setBackgroundEnabled(true);
        // NOLINTNEXTLINE
        judgeArea->setBackgroundColor(ImVec4(0.3F, 0.2F, 0.25F, 0.7F));
        judgeArea->addWidget(std::make_shared<Label>("判定区"));
        judgeArea->addWidget(std::make_shared<Label>("当前回合: 刘备"));

        // 添加图片示例 - 使用嵌入的资源
        // auto testImage = std::make_shared<Image>();
        // testImage->setImageFromMemory(embedded_1, embedded_1_len);
        // testImage->setFixedSize(200, 150);
        // judgeArea->addWidget(testImage);

        judgeArea->addStretch(1);

        // 战斗信息区
        auto battleLog = std::make_shared<VBoxLayout>();
        battleLog->setBackgroundEnabled(true);
        battleLog->setBackgroundColor(ImVec4(0.18F, 0.18F, 0.22F, 0.8F));
        battleLog->setMargins(5, 5, 5, 5);
        battleLog->addWidget(std::make_shared<Label>("战斗记录:"));
        battleLog->addWidget(std::make_shared<Label>("刘备对张飞使用【杀】"));
        battleLog->addWidget(std::make_shared<Label>("张飞使用【闪】"));
        battleLog->addStretch(1);

        centerArea->addWidget(judgeArea, 1);
        centerArea->addWidget(battleLog, 3);

        // 右侧：装备区
        auto rightSideLayout = std::make_shared<VBoxLayout>();
        rightSideLayout->setBackgroundEnabled(true);
        rightSideLayout->setBackgroundColor(ImVec4(0.30F, 0.22F, 0.15F, 0.85F));
        rightSideLayout->setFixedSize(150, 0);
        rightSideLayout->setMargins(10, 10, 10, 10);
        rightSideLayout->setSpacing(8);

        rightSideLayout->addWidget(std::make_shared<Label>("装备区"));
        rightSideLayout->addWidget(std::make_shared<Button>("武器: 青釭剑"));
        rightSideLayout->addWidget(std::make_shared<Button>("防具: 八卦阵"));
        rightSideLayout->addWidget(std::make_shared<Button>("坐骑: 的卢"));
        rightSideLayout->addStretch(1);

        middleLayout->addWidget(leftSideLayout);
        middleLayout->addWidget(centerArea, 1);
        middleLayout->addWidget(rightSideLayout);

        // ========== 底部区域：玩家手牌和信息 ==========
        auto bottomArea = std::make_shared<VBoxLayout>();
        bottomArea->setBackgroundEnabled(true);
        bottomArea->setBackgroundColor(ImVec4(0.25F, 0.30F, 0.25F, 0.85F));
        bottomArea->setSpacing(8);
        bottomArea->setMargins(10, 10, 10, 10);

        // 玩家信息栏
        auto playerInfoLayout = std::make_shared<HBoxLayout>();
        playerInfoLayout->setBackgroundEnabled(true);
        playerInfoLayout->setBackgroundColor(ImVec4(0.35F, 0.30F, 0.25F, 0.9F));
        playerInfoLayout->setMargins(8, 8, 8, 8);

        auto playerInfo = std::make_shared<VBoxLayout>();
        playerInfo->addWidget(std::make_shared<Label>("【刘备】 主公"));
        playerInfo->addWidget(std::make_shared<Label>("HP: 4/4 ♥♥♥♥"));

        auto skillsLayout = std::make_shared<HBoxLayout>();
        skillsLayout->addWidget(std::make_shared<Button>("仁德"));
        skillsLayout->addWidget(std::make_shared<Button>("激将"));

        playerInfoLayout->addWidget(playerInfo);
        playerInfoLayout->addWidget(skillsLayout);
        playerInfoLayout->addStretch(1);

        auto actionButtons = std::make_shared<HBoxLayout>();
        actionButtons->setSpacing(10);
        actionButtons->addStretch(1);
        actionButtons->addWidget(std::make_shared<Button>("出牌"));
        actionButtons->addWidget(std::make_shared<Button>("结束回合"));
        actionButtons->addWidget(std::make_shared<Button>("取消"));

        playerInfoLayout->addWidget(actionButtons);

        // 手牌区
        auto handCardsLayout = std::make_shared<VBoxLayout>();
        handCardsLayout->addWidget(std::make_shared<Label>("手牌 (5张):"));

        auto cardsRow = std::make_shared<HBoxLayout>();
        cardsRow->setSpacing(8);
        cardsRow->addStretch(1);

        // 添加5张手牌
        auto card1 = std::make_shared<Button>("♠杀");
        card1->setFixedSize(80, 110);
        auto card2 = std::make_shared<Button>("♥桃");
        card2->setFixedSize(80, 110);
        auto card3 = std::make_shared<Button>("♦闪");
        card3->setFixedSize(80, 110);
        auto card4 = std::make_shared<Button>("♣决斗");
        card4->setFixedSize(80, 110);
        auto card5 = std::make_shared<Button>("♠无懈");
        card5->setFixedSize(80, 110);

        cardsRow->addWidget(card1);
        cardsRow->addWidget(card2);
        cardsRow->addWidget(card3);
        cardsRow->addWidget(card4);
        cardsRow->addWidget(card5);
        cardsRow->addStretch(1);

        handCardsLayout->addWidget(cardsRow);

        bottomArea->addWidget(playerInfoLayout);
        bottomArea->addWidget(handCardsLayout);

        // 组合所有区域
        mainLayout->addWidget(topOpponentsLayout);
        mainLayout->addWidget(middleLayout, 1);
        mainLayout->addWidget(bottomArea);

        m_rootLayout = mainLayout;
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
        // NOLINTNEXTLINE
        SDL_SetRenderDrawColor(m_renderer, 30, 30, 30, 255);
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
        static const ImWchar ranges[] = {
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
                                          ranges);

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