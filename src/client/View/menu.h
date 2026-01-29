#pragma once

#include <ui.hpp>
#include "Mainwindow.h"

namespace client::view
{

/**
 * @brief 创建初始菜单对话框
 */
inline void CreateMenuDialog()
{
    auto view = ui::Registry::View<ui::components::BaseInfo>();
    for (auto entity : view)
    {
        if (view.get<ui::components::BaseInfo>(entity).alias == "menuDialog")
        {
            return; // 已存在，直接返回
        }
    }
    // 创建菜单对话框
    auto menuDialog = ui::factory::CreateDialog("PestManKill Menu", "menuDialog");

    ui::size::SetSize(menuDialog, 160.0F, 300.0F);

    // 添加窗口背景
    ui::visibility::SetBackgroundColor(menuDialog, {0.15F, 0.15F, 0.15F, 0.95F});
    ui::visibility::SetBorderRadius(menuDialog, 8.0F);

    // 设置垂直布局
    ui::layout::SetLayoutDirection(menuDialog, ui::policies::LayoutDirection::VERTICAL);
    ui::layout::SetLayoutSpacing(menuDialog, 15.0F);

    // 设置内边距
    ui::layout::SetPadding(menuDialog, 20.0F);

    // 创建标题标签
    auto titleLabel = ui::factory::CreateLabel("欢迎来到 害虫杀", "titleLabel");
    ui::text::SetTextAlignment(titleLabel, ui::policies::Alignment::CENTER);
    ui::text::SetTextColor(titleLabel, {1.0F, 0.9F, 0.3F, 1.0F}); // 金黄色
    ui::hierarchy::AddChild(menuDialog, titleLabel);

    // 创建分隔间距
    auto spacer1 = ui::factory::CreateSpacer(1, "spacer1");
    ui::hierarchy::AddChild(menuDialog, spacer1);

    // 创建开始游戏按钮
    auto startBtn = ui::factory::CreateButton("开始", "startBtn");
    ui::size::SetFixedSize(startBtn, 150.0F, 40.0F);
    ui::text::SetTextAlignment(startBtn, ui::policies::Alignment::CENTER);
    ui::visibility::SetBackgroundColor(startBtn, {0.2F, 0.4F, 0.8F, 1.0F});
    ui::visibility::SetBorderRadius(startBtn, 5.0F);
    ui::visibility::SetBorderColor(startBtn, {0.4F, 0.6F, 1.0F, 1.0F});
    ui::visibility::SetBorderThickness(startBtn, 2.0F);

    // 点击开始游戏，创建主窗口
    ui::text::SetClickCallback(startBtn,
                               [menuDialog]()
                               {
                                   CreateMainWindow();
                                   // 销毁菜单对话框

                                   ui::utils::CloseWindow(menuDialog);
                               });
    ui::hierarchy::AddChild(menuDialog, startBtn);

    // 创建设置按钮
    auto settingsBtn = ui::factory::CreateButton("设置", "settingsBtn");
    ui::size::SetFixedSize(settingsBtn, 150.0F, 40.0F);
    ui::text::SetTextAlignment(settingsBtn, ui::policies::Alignment::CENTER);
    ui::visibility::SetBackgroundColor(settingsBtn, {0.3F, 0.3F, 0.3F, 1.0F});
    ui::visibility::SetBorderRadius(settingsBtn, 5.0F);
    ui::visibility::SetBorderColor(settingsBtn, {0.5F, 0.5F, 0.5F, 1.0F});
    ui::visibility::SetBorderThickness(settingsBtn, 2.0F);
    ui::hierarchy::AddChild(menuDialog, settingsBtn);

    // 创建退出按钮
    auto exitBtn = ui::factory::CreateButton("退出", "exitBtn");
    ui::size::SetFixedSize(exitBtn, 150.0F, 40.0F);
    ui::text::SetTextAlignment(exitBtn, ui::policies::Alignment::CENTER);
    ui::visibility::SetBackgroundColor(exitBtn, {0.6F, 0.2F, 0.2F, 1.0F});
    ui::visibility::SetBorderRadius(exitBtn, 5.0F);
    ui::visibility::SetBorderColor(exitBtn, {0.8F, 0.3F, 0.3F, 1.0F});
    ui::visibility::SetBorderThickness(exitBtn, 2.0F);
    ui::text::SetClickCallback(exitBtn,
                               []()
                               {
                                   LOG_INFO("退出menu.");
                                   ui::utils::QuitUiEventLoop();
                               });
    ui::hierarchy::AddChild(menuDialog, exitBtn);

    // 创建底部间距
    auto spacer2 = ui::factory::CreateSpacer(1, "spacer2");
    ui::hierarchy::AddChild(menuDialog, spacer2);

    // 创建版本信息标签
    auto versionLabel = ui::factory::CreateLabel("v0.1.0 - 2026", "versionLabel");
    ui::text::SetTextAlignment(versionLabel, ui::policies::Alignment::CENTER);
    ui::text::SetTextColor(versionLabel, {0.6F, 0.6F, 0.6F, 1.0F});
    ui::hierarchy::AddChild(menuDialog, versionLabel);

    // 显示菜单对话框（同步尺寸并居中）
    LOG_INFO("Showing menu dialog...");
    ui::visibility::Show(menuDialog);
    LOG_INFO("CreateMenuDialog completed.");
}

} // namespace client::view
