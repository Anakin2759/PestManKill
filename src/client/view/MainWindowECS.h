/**
 * ************************************************************************
 *
 * @file MainWindowECS.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief 使用ECS架构的主窗口示例
 *
 * 展示如何使用新的ECS UI系统创建完整的应用程序
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include "src/client/ui/ECSApplication.h"
#include "src/client/model/UIHelper.h"
#include "src/client/utils/Logger.h"
#include "src/client/utils/Dispatcher.h"

namespace ui
{

/**
 * @brief 使用ECS架构的主窗口
 */
class MainWindowECS : public ECSApplication
{
public:
    MainWindowECS() : ECSApplication("PestManKill - ECS UI Demo", 1200, 800) { setupUI(); }

protected:
    void setupUI() override
    {
        auto& factory = getUiSystem().getFactory();
        auto& registry = getUiSystem().getRegistry();
        auto rootEntity = getRootEntity();

        // 设置根布局
        auto& rootLayout = registry.get<components::Layout>(rootEntity);
        rootLayout.spacing = 10.0F;
        rootLayout.margins = ImVec4(20.0F, 20.0F, 20.0F, 20.0F);

        // ===================== 创建标题栏 =====================
        auto titleLabel = factory.createLabel("Welcome to PestManKill");
        auto& titleLabelComp = registry.get<components::Label>(titleLabel);
        titleLabelComp.textColor = ImVec4(1.0F, 0.8F, 0.0F, 1.0F);
        helper::setFixedSize(registry, titleLabel, 0.0F, 40.0F);
        factory.addWidgetToLayout(rootEntity, titleLabel, 0);

        // ===================== 创建主内容区 =====================
        auto contentLayout = factory.createHBoxLayout();
        helper::setLayoutSpacing(registry, contentLayout, 15.0F);
        factory.addWidgetToLayout(rootEntity, contentLayout, 1);

        // 左侧面板
        createLeftPanel(contentLayout);

        // 中间内容
        createCenterPanel(contentLayout);

        // 右侧面板
        createRightPanel(contentLayout);

        // ===================== 创建底部状态栏 =====================
        auto statusLayout = factory.createHBoxLayout();
        helper::setLayoutSpacing(registry, statusLayout, 10.0F);
        factory.addWidgetToLayout(rootEntity, statusLayout, 0);

        m_statusLabel = factory.createLabel("Ready");
        auto& statusLabelComp = registry.get<components::Label>(m_statusLabel);
        statusLabelComp.textColor = ImVec4(0.7F, 0.7F, 0.7F, 1.0F);
        factory.addWidgetToLayout(statusLayout, m_statusLabel, 1);

        // ===================== 设置事件监听 =====================
        setupEventHandlers();
    }

    void handleEvent(const SDL_Event& event) override
    {
        // 处理自定义SDL事件
        if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_ESCAPE)
            {
                utils::LOG_INFO("ESC key pressed");
            }
        }
    }

private:
    void createLeftPanel(entt::entity parentLayout)
    {
        auto& factory = getUiSystem().getFactory();
        auto& registry = getUiSystem().getRegistry();

        auto leftPanel = factory.createVBoxLayout();
        helper::setFixedSize(registry, leftPanel, 250.0F, 0.0F);
        helper::setLayoutSpacing(registry, leftPanel, 8.0F);
        helper::setBackgroundColor(registry, leftPanel, ImVec4(0.15F, 0.15F, 0.2F, 1.0F));
        factory.addWidgetToLayout(parentLayout, leftPanel, 0);

        // 标题
        auto panelTitle = factory.createLabel("Menu");
        auto& titleComp = registry.get<components::Label>(panelTitle);
        titleComp.textColor = ImVec4(0.8F, 0.9F, 1.0F, 1.0F);
        factory.addWidgetToLayout(leftPanel, panelTitle, 0);

        // 按钮组
        auto startButton = factory.createButton("Start Game", [this]() { onStartGame(); });
        factory.addWidgetToLayout(leftPanel, startButton, 0);

        auto settingsButton = factory.createButton("Settings", [this]() { onSettings(); });
        factory.addWidgetToLayout(leftPanel, settingsButton, 0);

        auto exitButton = factory.createButton("Exit", [this]() { onExit(); });
        factory.addWidgetToLayout(leftPanel, exitButton, 0);

        // 弹性空间
        factory.addStretchToLayout(leftPanel, 1);
    }

    void createCenterPanel(entt::entity parentLayout)
    {
        auto& factory = getUiSystem().getFactory();
        auto& registry = getUiSystem().getRegistry();

        auto centerPanel = factory.createVBoxLayout();
        helper::setLayoutSpacing(registry, centerPanel, 10.0F);
        helper::setBackgroundColor(registry, centerPanel, ImVec4(0.1F, 0.12F, 0.15F, 1.0F));
        factory.addWidgetToLayout(parentLayout, centerPanel, 1);

        // 标题
        auto panelTitle = factory.createLabel("Game Area");
        auto& titleComp = registry.get<components::Label>(panelTitle);
        titleComp.textColor = ImVec4(0.8F, 0.9F, 1.0F, 1.0F);
        factory.addWidgetToLayout(centerPanel, panelTitle, 0);

        // 内容文本
        m_contentLabel = factory.createLabel("Select an option from the menu...");
        auto& contentComp = registry.get<components::Label>(m_contentLabel);
        contentComp.wordWrap = true;
        contentComp.textColor = ImVec4(0.9F, 0.9F, 0.9F, 1.0F);
        factory.addWidgetToLayout(centerPanel, m_contentLabel, 1);

        // 输入框
        auto inputLayout = factory.createHBoxLayout();
        helper::setLayoutSpacing(registry, inputLayout, 10.0F);
        factory.addWidgetToLayout(centerPanel, inputLayout, 0);

        m_textInput = factory.createTextEdit("Type here...", false);
        auto& textEditComp = registry.get<components::TextEdit>(m_textInput);
        textEditComp.maxLength = 256;
        textEditComp.onTextChanged = [this](const std::string& text) { onTextChanged(text); };
        factory.addWidgetToLayout(inputLayout, m_textInput, 1);

        auto sendButton = factory.createButton("Send", [this]() { onSendMessage(); });
        factory.addWidgetToLayout(inputLayout, sendButton, 0);
    }

    void createRightPanel(entt::entity parentLayout)
    {
        auto& factory = getUiSystem().getFactory();
        auto& registry = getUiSystem().getRegistry();

        auto rightPanel = factory.createVBoxLayout();
        helper::setFixedSize(registry, rightPanel, 200.0F, 0.0F);
        helper::setLayoutSpacing(registry, rightPanel, 8.0F);
        helper::setBackgroundColor(registry, rightPanel, ImVec4(0.15F, 0.15F, 0.2F, 1.0F));
        factory.addWidgetToLayout(parentLayout, rightPanel, 0);

        // 标题
        auto panelTitle = factory.createLabel("Info");
        auto& titleComp = registry.get<components::Label>(panelTitle);
        titleComp.textColor = ImVec4(0.8F, 0.9F, 1.0F, 1.0F);
        factory.addWidgetToLayout(rightPanel, panelTitle, 0);

        // 信息标签
        m_infoLabel = factory.createLabel("No information available");
        auto& infoComp = registry.get<components::Label>(m_infoLabel);
        infoComp.wordWrap = true;
        infoComp.textColor = ImVec4(0.7F, 0.7F, 0.7F, 1.0F);
        factory.addWidgetToLayout(rightPanel, m_infoLabel, 1);
    }

    void setupEventHandlers()
    {
        auto& dispatcher = utils::Dispatcher::getInstance();

        // 监听按钮点击事件
        dispatcher.sink<events::ButtonClicked>().connect<&MainWindowECS::onButtonClicked>(this);

        // 监听文本改变事件
        dispatcher.sink<events::TextChanged>().connect<&MainWindowECS::onTextChangedEvent>(this);
    }

    // ===================== 事件处理 =====================
    void onButtonClicked(const events::ButtonClicked& event)
    {
        utils::LOG_INFO("Button clicked: entity {}", static_cast<uint32_t>(event.entity));
        updateStatus("Button clicked");
    }

    void onTextChangedEvent(const events::TextChanged& event)
    {
        utils::LOG_INFO("Text changed event: {}", event.newText);
    }

    void onStartGame()
    {
        utils::LOG_INFO("Start Game clicked");
        updateStatus("Game started!");

        auto& registry = getUiSystem().getRegistry();
        auto& contentLabel = registry.get<components::Label>(m_contentLabel);
        contentLabel.text = "Game is starting...\n\nPrepare yourself for battle!";

        // 示例：启动淡入动画
        helper::setAlpha(registry, m_contentLabel, 0.0F);
        helper::startAlphaAnimation(registry, m_contentLabel, 0.0F, 1.0F, 1.0F);
    }

    void onSettings()
    {
        utils::LOG_INFO("Settings clicked");
        updateStatus("Opening settings...");

        auto& registry = getUiSystem().getRegistry();
        auto& infoLabel = registry.get<components::Label>(m_infoLabel);
        infoLabel.text = "Settings panel would appear here.\n\nConfigure your game preferences.";
    }

    void onExit()
    {
        utils::LOG_INFO("Exit clicked");
        updateStatus("Exiting...");

        // 触发退出事件
        SDL_Event quitEvent;
        quitEvent.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&quitEvent);
    }

    void onTextChanged(const std::string& text) { utils::LOG_INFO("Text input changed: {}", text); }

    void onSendMessage()
    {
        auto& registry = getUiSystem().getRegistry();
        auto& textEdit = registry.get<components::TextEdit>(m_textInput);

        std::string message = textEdit.text;
        if (!message.empty())
        {
            utils::LOG_INFO("Sending message: {}", message);
            updateStatus("Message sent: " + message);

            // 更新内容区域
            auto& contentLabel = registry.get<components::Label>(m_contentLabel);
            contentLabel.text = "You sent: " + message;

            // 清空输入框
            textEdit.text.clear();
        }
    }

    void updateStatus(const std::string& status)
    {
        auto& registry = getUiSystem().getRegistry();
        auto& statusLabel = registry.get<components::Label>(m_statusLabel);
        statusLabel.text = status;
    }

private:
    entt::entity m_statusLabel = entt::null;
    entt::entity m_contentLabel = entt::null;
    entt::entity m_infoLabel = entt::null;
    entt::entity m_textInput = entt::null;
};

} // namespace ui
