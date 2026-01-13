/**
 * ************************************************************************
 *
 * @file main.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.2
 * @brief 客户端主程序（集成 UI 和 Net 模块）
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#include <iostream>
#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

// 工具
#include <utils.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "src/utils/Functions.h"

#include "src/ui/core/Application.h"
#include "src/ui/core/Factory.h"
#include "src/ui/core/Helper.h"
#include "src/ui/common/Types.h"
namespace
{
// 前向声明
void CreateMainWindow();
void CreateMenuDialog();

/**
 * @brief 创建主窗口（空窗口，点击开始游戏后显示）
 */
void CreateMainWindow()
{
    auto& registry = utils::Registry::getInstance();

    // 创建主游戏窗口（使用 FillParent 策略自动填满）
    auto gameWindow = ui::factory::CreateWindow("Game", "gameWindow");
    auto& sizeComp = registry.get<ui::components::Size>(gameWindow);
    sizeComp.widthPolicy = ui::policies::Size::FillParent;
    sizeComp.heightPolicy = ui::policies::Size::FillParent;

    // 设置标题栏
    auto& windowComp = registry.get<ui::components::Window>(gameWindow);
    windowComp.hasTitleBar = false; // 全屏模式不需要标题栏
    windowComp.noResize = true;
    windowComp.noMove = true;

    // 添加窗口背景
    auto& gameBg = registry.emplace<ui::components::Background>(gameWindow);
    gameBg.color = {0.1f, 0.1f, 0.12f, 1.0f};
    gameBg.borderRadius = 4.0f;
    gameBg.enabled = true;

    // 设置布局
    auto& layout = registry.get<ui::components::LayoutInfo>(gameWindow);
    layout.direction = ui::policies::LayoutDirection::VERTICAL;
    layout.spacing = 10.0f;

    // 设置内边距
    auto& padding = registry.get<ui::components::Padding>(gameWindow);
    padding.values = {10.0f, 10.0f, 10.0f, 10.0f};

    // 添加一个提示标签
    auto infoLabel = ui::factory::CreateLabel("主界面 - 待开发", "gameInfoLabel");
    auto& infoText = registry.get<ui::components::Text>(infoLabel);
    infoText.alignment = ui::policies::Alignment::CENTER;
    infoText.color = {0.8f, 0.8f, 0.8f, 1.0f};
    ui::helper::addChild(gameWindow, infoLabel);

    auto openMenuDialogBtn = ui::factory::CreateButton("打开菜单", "openMenuDialogBtn");

    ui::helper::addChild(gameWindow, openMenuDialogBtn);

    LOG_INFO("主窗口已创建");
}

/**
 * @brief 创建菜单对话框（启动时显示的主菜单）
 */
void CreateMenuDialog()
{
    auto& registry = utils::Registry::getInstance();
    auto view = registry.view<ui::components::BaseInfo>();
    for (auto entity : view)
    {
        if (view.get<ui::components::BaseInfo>(entity).alias == "menuDialog")
        {
            return; // 已存在，直接返回
        }
    }
    // 创建菜单对话框
    auto menuDialog = ui::factory::CreateDialog("PestManKill Menu", "menuDialog");
    registry.get<ui::components::Size>(menuDialog).size = {400.0f, 300.0f};
    // 位置 (0,0) 会触发自动居中

    // 隐藏标题栏
    auto& dialogComp = registry.get<ui::components::Dialog>(menuDialog);
    dialogComp.hasTitleBar = false;

    // 添加窗口背景
    auto& mainBg = registry.emplace<ui::components::Background>(menuDialog);
    mainBg.color = {0.15f, 0.15f, 0.15f, 0.95f};
    mainBg.borderRadius = 8.0f;
    mainBg.enabled = true;

    // 设置垂直布局
    auto& layout = registry.get<ui::components::LayoutInfo>(menuDialog);
    layout.direction = ui::policies::LayoutDirection::VERTICAL;
    layout.spacing = 15.0f;

    // 设置内边距
    auto& padding = registry.get<ui::components::Padding>(menuDialog);
    padding.values = {20.0f, 20.0f, 20.0f, 20.0f};

    // 创建标题标签
    auto titleLabel = ui::factory::CreateLabel("欢迎来到 PestManKill", "titleLabel");
    auto& titleText = registry.get<ui::components::Text>(titleLabel);
    titleText.alignment = ui::policies::Alignment::CENTER;
    titleText.color = {1.0f, 0.9f, 0.3f, 1.0f}; // 金黄色
    ui::helper::addChild(menuDialog, titleLabel);

    // 创建分隔间距
    auto spacer1 = ui::factory::CreateSpacer(1, "spacer1");
    ui::helper::addChild(menuDialog, spacer1);

    // 创建开始游戏按钮
    auto startBtn = ui::factory::CreateButton("开始游戏", "startBtn");
    ui::helper::setFixedSize(startBtn, 150.0f, 40.0f);
    auto& startText = registry.get<ui::components::Text>(startBtn);
    startText.alignment = ui::policies::Alignment::CENTER;
    auto& startBg = registry.emplace<ui::components::Background>(startBtn);
    startBg.color = {0.2f, 0.4f, 0.8f, 1.0f};
    startBg.borderRadius = 5.0f;
    startBg.enabled = true;
    auto& startBorder = registry.emplace<ui::components::Border>(startBtn);
    startBorder.color = {0.4f, 0.6f, 1.0f, 1.0f};
    startBorder.thickness = 2.0f;
    startBorder.borderRadius = 5.0f;
    // 点击开始游戏，创建主窗口
    auto& startClickable = registry.get<ui::components::Clickable>(startBtn);
    startClickable.onClick = []()
    {
        CreateMainWindow();
        // auto hide
    };
    ui::helper::addChild(menuDialog, startBtn);

    // 创建设置按钮
    auto settingsBtn = ui::factory::CreateButton("设置", "settingsBtn");
    ui::helper::setFixedSize(settingsBtn, 150.0f, 40.0f);
    auto& settingsText = registry.get<ui::components::Text>(settingsBtn);
    settingsText.alignment = ui::policies::Alignment::CENTER;
    auto& settingsBg = registry.emplace<ui::components::Background>(settingsBtn);
    settingsBg.color = {0.3f, 0.3f, 0.3f, 1.0f};
    settingsBg.borderRadius = 5.0f;
    settingsBg.enabled = true;
    auto& settingsBorder = registry.emplace<ui::components::Border>(settingsBtn);
    settingsBorder.color = {0.5f, 0.5f, 0.5f, 1.0f};
    settingsBorder.thickness = 2.0f;
    settingsBorder.borderRadius = 5.0f;
    ui::helper::addChild(menuDialog, settingsBtn);

    // 创建退出按钮
    auto exitBtn = ui::factory::CreateButton("退出", "exitBtn");
    ui::helper::setFixedSize(exitBtn, 150.0f, 40.0f);
    auto& exitText = registry.get<ui::components::Text>(exitBtn);
    exitText.alignment = ui::policies::Alignment::CENTER;
    auto& exitBg = registry.emplace<ui::components::Background>(exitBtn);
    exitBg.color = {0.6f, 0.2f, 0.2f, 1.0f};
    exitBg.borderRadius = 5.0f;
    exitBg.enabled = true;
    auto& exitBorder = registry.emplace<ui::components::Border>(exitBtn);
    exitBorder.color = {0.8f, 0.3f, 0.3f, 1.0f};
    exitBorder.thickness = 2.0f;
    exitBorder.borderRadius = 5.0f;
    auto& exitClickable = registry.get<ui::components::Clickable>(exitBtn);
    exitClickable.onClick = []()
    {
        std::cout << "退出按钮被点击，关闭应用程序。" << std::endl;
        auto& dispatcher = utils::Dispatcher::getInstance();
        dispatcher.enqueue<ui::events::QuitRequested>(ui::events::QuitRequested{});
    };
    ui::helper::addChild(menuDialog, exitBtn);

    // 创建底部间距
    auto spacer2 = ui::factory::CreateSpacer(1, "spacer2");
    ui::helper::addChild(menuDialog, spacer2);

    // 创建版本信息标签
    auto versionLabel = ui::factory::CreateLabel("v0.1.0 - 2026", "versionLabel");
    auto& versionText = registry.get<ui::components::Text>(versionLabel);
    versionText.alignment = ui::policies::Alignment::CENTER;
    versionText.color = {0.6f, 0.6f, 0.6f, 1.0f};
    ui::helper::addChild(menuDialog, versionLabel);
}
} // namespace
int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    utils::functions::setConsoleToUTF8();
    try
    {
        // 创建并运行 UI 应用程序
        ui::Application app("PestManKill Client", 1024, 768);

        // 创建菜单对话框
        CreateMenuDialog();

        app.exec();
    }
    catch (const std::exception& e)
    {
        std::cerr << "应用程序异常终止: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}