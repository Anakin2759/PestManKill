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
        auto& registry = utils::Registry::getInstance();
        auto rootEntity = getRootEntity();

        // 设置根布局
        auto& rootLayout = registry.get<components::Layout>(rootEntity);
        rootLayout.spacing = 10.0F;
        rootLayout.margins = ImVec4(20.0F, 20.0F, 20.0F, 20.0F);

        // ===================== 创建标题栏 =====================
        auto titleLabel = factory.createLabel("Welcome to PestManKill");
        auto& titleText = registry.emplace<components::ShowText>(titleLabel);
        titleText.text = "Welcome to PestManKill";
        titleText.textColor = ImVec4(1.0F, 0.8F, 0.0F, 1.0F);
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
        auto& statusText = registry.emplace<components::ShowText>(m_statusLabel);
        statusText.text = "Ready";
        statusText.textColor = ImVec4(0.7F, 0.7F, 0.7F, 1.0F);
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
                LOG_INFO("ESC key pressed");
            }
        }
    }

private:
    void createLeftPanel(entt::entity parentLayout)
    {
        auto& factory = getUiSystem().getFactory();
        auto& registry = utils::Registry::getInstance();

        auto leftPanel = factory.createVBoxLayout();
        helper::setFixedSize(registry, leftPanel, 250.0F, 0.0F);
        helper::setLayoutSpacing(registry, leftPanel, 8.0F);
        helper::setBackgroundColor(registry, leftPanel, ImVec4(0.15F, 0.15F, 0.2F, 1.0F));
        factory.addWidgetToLayout(parentLayout, leftPanel, 0);

        // 标题
        auto panelTitle = factory.createLabel("Menu");
        auto& titleText = registry.emplace<components::ShowText>(panelTitle);
        titleText.text = "Menu";
        titleText.textColor = ImVec4(0.8F, 0.9F, 1.0F, 1.0F);
        factory.addWidgetToLayout(leftPanel, panelTitle, 0);

        // 按钮组
        m_startButton = factory.createButton("Start Game");
        registry.get<components::Clickable>(m_startButton).text = "Start Game";
        factory.addWidgetToLayout(leftPanel, m_startButton, 0);

        m_settingsButton = factory.createButton("Settings");
        registry.get<components::Clickable>(m_settingsButton).text = "Settings";
        factory.addWidgetToLayout(leftPanel, m_settingsButton, 0);

        m_exitButton = factory.createButton("Exit");
        registry.get<components::Clickable>(m_exitButton).text = "Exit";
        factory.addWidgetToLayout(leftPanel, m_exitButton, 0);

        // 弹性空间
        factory.addStretchToLayout(leftPanel, 1);
    }

    void createCenterPanel(entt::entity parentLayout)
    {
        auto& factory = getUiSystem().getFactory();
        auto& registry = utils::Registry::getInstance();

        auto centerPanel = factory.createVBoxLayout();
        helper::setLayoutSpacing(registry, centerPanel, 10.0F);
        helper::setBackgroundColor(registry, centerPanel, ImVec4(0.1F, 0.12F, 0.15F, 1.0F));
        factory.addWidgetToLayout(parentLayout, centerPanel, 1);

        // 标题
        auto panelTitle = factory.createLabel("Game Area");
        auto& titleText = registry.emplace<components::ShowText>(panelTitle);
        titleText.text = "Game Area";
        titleText.textColor = ImVec4(0.8F, 0.9F, 1.0F, 1.0F);
        factory.addWidgetToLayout(centerPanel, panelTitle, 0);

        // 内容文本
        m_contentLabel = factory.createLabel("Select an option from the menu...");
        auto& contentText = registry.emplace<components::ShowText>(m_contentLabel);
        contentText.text = "Select an option from the menu...";
        contentText.wordWrap = true;
        contentText.textColor = ImVec4(0.9F, 0.9F, 0.9F, 1.0F);
        factory.addWidgetToLayout(centerPanel, m_contentLabel, 1);

        // 输入框
        auto inputLayout = factory.createHBoxLayout();
        helper::setLayoutSpacing(registry, inputLayout, 10.0F);
        factory.addWidgetToLayout(centerPanel, inputLayout, 0);

        m_textInput = factory.createTextEdit("Type here...", false);
        auto& textEditComp = registry.get<components::TextEdit>(m_textInput);
        textEditComp.placeholder = "Type here...";
        textEditComp.maxLength = 256;
        textEditComp.onTextChanged = [this](const std::string& text) { onTextChanged(text); };
        factory.addWidgetToLayout(inputLayout, m_textInput, 1);

        m_sendButton = factory.createButton("Send");
        registry.get<components::Clickable>(m_sendButton).text = "Send";
        factory.addWidgetToLayout(inputLayout, m_sendButton, 0);
    }

    void createRightPanel(entt::entity parentLayout)
    {
        auto& factory = getUiSystem().getFactory();
        auto& registry = utils::Registry::getInstance();

        auto rightPanel = factory.createVBoxLayout();
        helper::setFixedSize(registry, rightPanel, 200.0F, 0.0F);
        helper::setLayoutSpacing(registry, rightPanel, 8.0F);
        helper::setBackgroundColor(registry, rightPanel, ImVec4(0.15F, 0.15F, 0.2F, 1.0F));
        factory.addWidgetToLayout(parentLayout, rightPanel, 0);

        // 标题
        auto panelTitle = factory.createLabel("Info");
        auto& titleText = registry.emplace<components::ShowText>(panelTitle);
        titleText.text = "Info";
        titleText.textColor = ImVec4(0.8F, 0.9F, 1.0F, 1.0F);
        factory.addWidgetToLayout(rightPanel, panelTitle, 0);

        // 信息标签
        m_infoLabel = factory.createLabel("No information available");
        auto& infoText = registry.emplace<components::ShowText>(m_infoLabel);
        infoText.text = "No information available";
        infoText.wordWrap = true;
        infoText.textColor = ImVec4(0.7F, 0.7F, 0.7F, 1.0F);
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
        LOG_INFO("Button clicked: entity {}", static_cast<uint32_t>(event.entity));

        // 根据按钮实体ID调用对应的处理函数
        if (event.entity == m_startButton)
        {
            onStartGame();
        }
        else if (event.entity == m_settingsButton)
        {
            onSettings();
        }
        else if (event.entity == m_exitButton)
        {
            onExit();
        }
        else if (event.entity == m_sendButton)
        {
            onSendMessage();
        }
        else
        {
            updateStatus("Button clicked");
        }
    }

    void onTextChangedEvent(const events::TextChanged& event) { LOG_INFO("Text changed event: {}", event.newText); }

    void onStartGame()
    {
        LOG_INFO("Start Game clicked");
        updateStatus("Game started!");

        auto& registry = utils::Registry::getInstance();
        auto& contentText = registry.get<components::ShowText>(m_contentLabel);
        contentText.text = "Game is starting...\n\nPrepare yourself for battle!";

        // 示例：启动淡入动画
        helper::setAlpha(registry, m_contentLabel, 0.0F);
        helper::startAlphaAnimation(registry, m_contentLabel, 0.0F, 1.0F, 1.0F);
    }

    void onSettings()
    {
        LOG_INFO("Settings clicked");
        updateStatus("Opening settings...");

        auto& registry = utils::Registry::getInstance();
        auto& infoText = registry.get<components::ShowText>(m_infoLabel);
        infoText.text = "Settings panel would appear here.\n\nConfigure your game preferences.";
    }

    void onExit()
    {
        LOG_INFO("Exit clicked");
        updateStatus("Exiting...");

        // 触发退出事件
        SDL_Event quitEvent;
        quitEvent.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&quitEvent);
    }

    void onTextChanged(const std::string& text) { LOG_INFO("Text input changed: {}", text); }

    void onSendMessage()
    {
        auto& registry = utils::Registry::getInstance();
        auto& textEdit = registry.get<components::TextEdit>(m_textInput);

        std::string message = textEdit.text;
        if (!message.empty())
        {
            LOG_INFO("Sending message: {}", message);
            updateStatus("Message sent: " + message);

            // 更新内容区域
            auto& contentText = registry.get<components::ShowText>(m_contentLabel);
            contentText.text = "You sent: " + message;

            // 清空输入框
            textEdit.text.clear();
        }
    }

    void updateStatus(const std::string& status)
    {
        auto& registry = utils::Registry::getInstance();
        auto& statusText = registry.get<components::ShowText>(m_statusLabel);
        statusText.text = status;
    }

private:
    // UI 实体
    entt::entity m_statusLabel = entt::null;
    entt::entity m_contentLabel = entt::null;
    entt::entity m_infoLabel = entt::null;
    entt::entity m_textInput = entt::null;

    // 按钮实体
    entt::entity m_startButton = entt::null;
    entt::entity m_settingsButton = entt::null;
    entt::entity m_exitButton = entt::null;
    entt::entity m_sendButton = entt::null;
};

} // namespace ui
