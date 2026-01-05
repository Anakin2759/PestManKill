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
#include <memory>
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

int main(int argc, char* argv[])
{
    utils::functions::setConsoleToUTF8();
    try
    {
        // 创建并运行 UI 应用程序
        ui::Application app("PestManKill Client", 1024, 768);

        // 创建极简主界面
        auto& registry = utils::Registry::getInstance();

        // 创建主窗口
        auto mainWindow = ui::factory::CreateWindow("PestManKill", "mainWindow");
        registry.get<ui::components::Size>(mainWindow).size = {400.0f, 300.0f};
        registry.get<ui::components::Position>(mainWindow).value = {312.0f, 234.0f}; // 居中

        // 隐藏标题栏
        auto& windowComp = registry.get<ui::components::Window>(mainWindow);
        windowComp.hasTitleBar = false;

        // 添加窗口背景
        auto& mainBg = registry.emplace<ui::components::Background>(mainWindow);
        mainBg.color = ImVec4(0.15f, 0.15f, 0.15f, 0.95f);
        mainBg.borderRadius = 8.0f;
        mainBg.enabled = true;

        // 设置垂直布局
        auto& layout = registry.get<ui::components::LayoutInfo>(mainWindow);
        layout.direction = ui::components::LayoutDirection::VERTICAL;
        layout.spacing = 15.0f;

        // 设置内边距
        auto& padding = registry.get<ui::components::Padding>(mainWindow);
        padding.values = {20.0f, 20.0f, 20.0f, 20.0f};

        // 创建标题标签
        auto titleLabel = ui::factory::CreateLabel("欢迎来到 PestManKill", "titleLabel");
        auto& titleText = registry.get<ui::components::Text>(titleLabel);
        titleText.alignment = ui::components::Alignment::CENTER;
        titleText.color = ImVec4(1.0f, 0.9f, 0.3f, 1.0f); // 金黄色
        // 设置固定宽度以便居中
        registry.get<ui::components::Size>(titleLabel).size = {360.0f, 30.0f};
        registry.get<ui::components::Size>(titleLabel).autoSize = false;
        ui::helper::addChild(mainWindow, titleLabel);

        // 创建分隔间距
        auto spacer1 = ui::factory::CreateSpacer(1, "spacer1");
        ui::helper::addChild(mainWindow, spacer1);

        // 创建开始游戏按钮
        auto startBtn = ui::factory::CreateButton("开始游戏", "startBtn");
        registry.get<ui::components::Size>(startBtn).size = {150.0f, 40.0f};
        registry.get<ui::components::Size>(startBtn).autoSize = false;
        registry.get<ui::components::Position>(startBtn).value = {125.0f, 0.0f}; // 水平居中 (400-150)/2 = 125
        // 设置文字居中
        auto& startText = registry.get<ui::components::Text>(startBtn);
        startText.alignment = ui::components::Alignment::CENTER;
        // 添加按钮样式
        auto& startBg = registry.emplace<ui::components::Background>(startBtn);
        startBg.color = ImVec4(0.2f, 0.4f, 0.8f, 1.0f); // 蓝色背景
        startBg.borderRadius = 5.0f;
        startBg.enabled = true;
        auto& startBorder = registry.emplace<ui::components::Border>(startBtn);
        startBorder.color = ImVec4(0.4f, 0.6f, 1.0f, 1.0f);
        startBorder.thickness = 2.0f;
        startBorder.borderRadius = 5.0f;
        ui::helper::addChild(mainWindow, startBtn);

        // 创建设置按钮
        auto settingsBtn = ui::factory::CreateButton("设置", "settingsBtn");
        registry.get<ui::components::Size>(settingsBtn).size = {150.0f, 40.0f};
        registry.get<ui::components::Size>(settingsBtn).autoSize = false;
        registry.get<ui::components::Position>(settingsBtn).value = {125.0f, 0.0f}; // 水平居中
        // 设置文字居中
        auto& settingsText = registry.get<ui::components::Text>(settingsBtn);
        settingsText.alignment = ui::components::Alignment::CENTER;
        // 添加按钮样式
        auto& settingsBg = registry.emplace<ui::components::Background>(settingsBtn);
        settingsBg.color = ImVec4(0.3f, 0.3f, 0.3f, 1.0f); // 灰色背景
        settingsBg.borderRadius = 5.0f;
        settingsBg.enabled = true;
        auto& settingsBorder = registry.emplace<ui::components::Border>(settingsBtn);
        settingsBorder.color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
        settingsBorder.thickness = 2.0f;
        settingsBorder.borderRadius = 5.0f;
        ui::helper::addChild(mainWindow, settingsBtn);

        // 创建退出按钮
        auto exitBtn = ui::factory::CreateButton("退出", "exitBtn");
        registry.get<ui::components::Size>(exitBtn).size = {150.0f, 40.0f};
        registry.get<ui::components::Size>(exitBtn).autoSize = false;
        registry.get<ui::components::Position>(exitBtn).value = {125.0f, 0.0f}; // 水平居中
        // 设置文字居中
        auto& exitText = registry.get<ui::components::Text>(exitBtn);
        exitText.alignment = ui::components::Alignment::CENTER;
        // 添加按钮样式
        auto& exitBg = registry.emplace<ui::components::Background>(exitBtn);
        exitBg.color = ImVec4(0.6f, 0.2f, 0.2f, 1.0f); // 红色背景
        exitBg.borderRadius = 5.0f;
        exitBg.enabled = true;
        auto& exitBorder = registry.emplace<ui::components::Border>(exitBtn);
        exitBorder.color = ImVec4(0.8f, 0.3f, 0.3f, 1.0f);
        exitBorder.thickness = 2.0f;
        exitBorder.borderRadius = 5.0f;
        // 添加点击事件
        auto& exitClickable = registry.get<ui::components::Clickable>(exitBtn);
        exitClickable.onClick = [](entt::entity)
        {
            auto& dispatcher = utils::Dispatcher::getInstance();
            dispatcher.trigger<ui::events::QuitRequested>(ui::events::QuitRequested{});
        };
        ui::helper::addChild(mainWindow, exitBtn);

        // 创建底部间距
        auto spacer2 = ui::factory::CreateSpacer(1, "spacer2");
        ui::helper::addChild(mainWindow, spacer2);

        // 创建版本信息标签
        auto versionLabel = ui::factory::CreateLabel("v0.1.0 - 2026", "versionLabel");
        auto& versionText = registry.get<ui::components::Text>(versionLabel);
        versionText.alignment = ui::components::Alignment::CENTER;
        versionText.color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f); // 灰色
        ui::helper::addChild(mainWindow, versionLabel);

        app.exec();
    }
    catch (const std::exception& e)
    {
        std::cerr << "应用程序异常终止: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}