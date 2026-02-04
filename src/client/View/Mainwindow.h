#pragma once

#include <vector>
#include <ui.hpp>
#include "src/utils/Logger.h"

namespace client::view
{

/**
 * @brief 创建主窗口（空窗口，点击开始游戏后显示）
 */
inline void CreateMainWindow()
{
    auto gameWindow = ui::factory::CreateWindow("Game", "gameWindow");
    ui::utils::SetWindowFlag(gameWindow, ui::policies::WindowFlag::Default);
    ui::size::SetSize(gameWindow, 1200.0F, 800.0F);

    ui::visibility::SetBackgroundColor(gameWindow, {0.1F, 0.1F, 0.12F, 1.0F});
    ui::visibility::SetBorderRadius(gameWindow, 4.0F);

    // 设置布局
    ui::layout::SetLayoutDirection(gameWindow, ui::policies::LayoutDirection::VERTICAL);
    ui::layout::SetLayoutSpacing(gameWindow, 10.0F);
    // 设置内边距
    ui::layout::SetPadding(gameWindow, 10.0F);

    // 填充空白区域
    auto mainSpacer = ui::factory::CreateSpacer(1, "mainSpacer");
    ui::hierarchy::AddChild(gameWindow, mainSpacer);

    // ===========================================
    // 聊天区域 (左下角游戏风格)
    // ===========================================
    // 1. 聊天总容器 (垂直布局：上面消息，下面输入)
    auto chatContainer = ui::factory::CreateVBoxLayout("chatContainer");

    // 设置背景
    ui::visibility::SetBackgroundColor(chatContainer, {0.05F, 0.05F, 0.08F, 0.8F});
    ui::visibility::SetBorderRadius(chatContainer, 4.0F);

    // 设置尺寸
    ui::size::SetFixedSize(chatContainer, 500.0F, 250.0F); // 增加高度
    ui::layout::SetLayoutSpacing(chatContainer, 5.0F);
    ui::layout::SetPadding(chatContainer, 5.0F);

    // 2. 消息显示区域 (占用大部分垂直空间)
    // 使用 TextBrowser（只读多行）展示消息
    const std::string initialMessages = "[System] Welcome to PestManKill!\n[System] Press Enter to send message.";
    auto messageArea = ui::factory::CreateTextBrowser(initialMessages, "", "messageArea");

    // 确保占满剩余空间
    ui::size::SetSizePolicy(messageArea, ui::policies::Size::FillParent);

    // 消息文本渲染设置
    ui::text::SetTextContent(messageArea, initialMessages);
    ui::text::SetTextWordWrap(messageArea, ui::policies::TextWrap::Word);
    ui::text::SetTextAlignment(messageArea, ui::policies::Alignment::TOP_LEFT);

    // 消息区域内边距
    ui::layout::SetPadding(messageArea, 4.0F);

    // 消息区域背景
    ui::visibility::SetBackgroundColor(messageArea, {0.08F, 0.08F, 0.1F, 0.5F});
    ui::visibility::SetBorderRadius(messageArea, 3.0F);
    // 消息区域边框
    ui::visibility::SetBorderColor(messageArea, {0.3F, 0.3F, 0.35F, 0.8F});
    ui::visibility::SetBorderThickness(messageArea, 1.0F);

    ui::hierarchy::AddChild(chatContainer, messageArea);

    // 3. 输入区域 (底部水平排列)
    auto inputRow = ui::factory::CreateHBoxLayout("inputRow");
    // 设置尺寸策略: HFill | VFixed
    ui::size::SetSizePolicy(inputRow, ui::policies::Size::HFill | ui::policies::Size::VFixed);
    ui::size::SetSize(inputRow, 0.0F, 30.0F); // Width由策略决定，Height固定30

    ui::layout::SetLayoutSpacing(inputRow, 5.0F);

    // 输入框 - 填充剩余宽度
    auto chatInput = ui::factory::CreateLineEdit("", "Say something...", "chatInput");
    ui::size::SetSizePolicy(chatInput, ui::policies::Size::HFill | ui::policies::Size::VFixed);
    // 输入框背景
    ui::visibility::SetBackgroundColor(chatInput, {0.15F, 0.15F, 0.18F, 0.9F});
    ui::visibility::SetBorderRadius(chatInput, 3.0F);

    // 输入框边框
    ui::visibility::SetBorderColor(chatInput, {0.3F, 0.3F, 0.35F, 1.0F});
    ui::visibility::SetBorderThickness(chatInput, 1.0F);
    // 注意：圆角已通过 SetBorderRadius 设置 (Background 和 Border 共享)

    // 发送按钮 - 固定宽度 (使用回车图标)
    auto sendBtn = ui::factory::CreateButton("", "sendBtn");
    ui::icon::SetIcon(sendBtn, "MaterialSymbols", 0xe31b, ui::policies::IconFlag::Default, 20.0f, 0.0f);
    ui::size::SetSizePolicy(sendBtn, ui::policies::Size::HFixed | ui::policies::Size::VFill);
    ui::size::SetSize(sendBtn, 40.0F, 0.0F); // 缩窄宽度以适配图标

    // 设置发送按钮背景
    ui::visibility::SetBackgroundColor(sendBtn, {0.2F, 0.5F, 0.8F, 1.0F}); // 蓝色背景
    ui::visibility::SetBorderRadius(sendBtn, 4.0F);

    // 设置发送按钮边框
    ui::visibility::SetBorderColor(sendBtn, {0.3F, 0.6F, 1.0F, 1.0F});
    ui::visibility::SetBorderThickness(sendBtn, 1.0F);
    // 发送事件
    ui::text::SetClickCallback(sendBtn,
                               [chatInput, messageArea]()
                               {
                                   // 获取输入框内容
                                   std::string content = ui::text::GetTextEditContent(chatInput);
                                   if (!content.empty())
                                   {
                                       LOG_INFO("发送聊天消息: {}", content);

                                       // 追加新消息到 TextBrowser
                                       const std::string fullMsg = "[Me]: " + content;
                                       std::string currentHistory = ui::text::GetTextEditContent(messageArea);

                                       if (!currentHistory.empty())
                                       {
                                           currentHistory += "\n";
                                       }
                                       currentHistory += fullMsg;

                                       // 限制消息行数 (简单实现)
                                       constexpr size_t MAX_MESSAGES = 20;
                                       // TODO: 这里可以优化，但保持原有逻辑
                                       std::vector<std::string> lines;
                                       lines.reserve(MAX_MESSAGES + 5);
                                       size_t start = 0;
                                       while (start <= currentHistory.size())
                                       {
                                           size_t end = currentHistory.find('\n', start);
                                           if (end == std::string::npos)
                                           {
                                               lines.emplace_back(currentHistory.substr(start));
                                               break;
                                           }
                                           lines.emplace_back(currentHistory.substr(start, end - start));
                                           start = end + 1;
                                       }

                                       if (lines.size() > MAX_MESSAGES)
                                       {
                                           const size_t keepFrom = lines.size() - MAX_MESSAGES;
                                           std::string trimmed;
                                           for (size_t i = keepFrom; i < lines.size(); ++i)
                                           {
                                               if (!trimmed.empty()) trimmed += "\n";
                                               trimmed += lines[i];
                                           }
                                           currentHistory = trimmed;
                                       }

                                       // 更新显示内容 (TextBrowser 通常同时使用 TextEdit.buffer 存储长文本 和
                                       // Text.content 显示)
                                       ui::text::SetTextEditContent(messageArea, currentHistory);
                                       ui::text::SetTextContent(messageArea, currentHistory);

                                       // 清空输入框
                                       ui::text::SetTextEditContent(chatInput, "");
                                       ui::text::SetTextContent(chatInput, "");

                                       // 文本内容变化但尺寸不变，只需标记渲染脏
                                       ui::utils::MarkRenderDirty(chatInput);
                                       ui::utils::MarkRenderDirty(messageArea);
                                   }
                               });

    ui::hierarchy::AddChild(inputRow, chatInput);
    ui::hierarchy::AddChild(inputRow, sendBtn);

    ui::hierarchy::AddChild(chatContainer, inputRow);

    // 将聊天面板添加到主窗口底部
    ui::hierarchy::AddChild(gameWindow, chatContainer);

    // 显示主窗口（同步尺寸并居中）
    ui::visibility::Show(gameWindow);

    LOG_INFO("主窗口已创建");
}

} // namespace client::view
