/**
 * ************************************************************************
 *
 * @file LoginScene.cpp
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.1
 * @brief 登录场景实现
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#include "SceneManager.h"
#include "GameClient.h"
#include "src/ui/ui/UIFactory.h"
#include "src/ui/ui/UIHelper.h"
#include "src/ui/components/UIComponents.h"

using namespace ui;

void LoginScene::enter()
{
    createUI();
}

void LoginScene::exit()
{
    if (m_registry.valid(m_rootEntity))
    {
        helper::traverseChildren(m_registry,
                                 m_rootEntity,
                                 [this](entt::entity entity)
                                 {
                                     if (m_registry.valid(entity))
                                     {
                                         m_registry.destroy(entity);
                                     }
                                 });
        m_registry.destroy(m_rootEntity);
        m_rootEntity = entt::null;
    }
}

void LoginScene::update(float deltaTime)
{
    // 场景特定的更新逻辑（如果需要）
}

void LoginScene::createUI()
{
    // 创建主容器（居中的垂直布局）
    m_rootEntity = factory::CreateVBox();
    helper::setFixedSize(m_registry, m_rootEntity, 400, 350);
    helper::setPadding(m_registry, m_rootEntity, 30);
    helper::setBackgroundColor(m_registry, m_rootEntity, ImVec4(0.12f, 0.12f, 0.15f, 0.95f));
    helper::setBorderRadius(m_registry, m_rootEntity, 12.0f);
    helper::setSpacing(m_registry, m_rootEntity, 12.0f);

    // 标题
    auto title = factory::CreateLabel("PestManKill");
    auto& titleText = m_registry.get<components::Text>(title);
    titleText.fontSize = 28.0f;
    titleText.color = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    titleText.alignment = components::Alignment::CENTER;
    helper::addChild(m_registry, m_rootEntity, title);

    // 副标题
    auto subtitle = factory::CreateLabel("Welcome Back!");
    auto& subtitleText = m_registry.get<components::Text>(subtitle);
    subtitleText.fontSize = 14.0f;
    subtitleText.color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
    subtitleText.alignment = components::Alignment::CENTER;
    helper::addChild(m_registry, m_rootEntity, subtitle);

    // 间隔
    auto spacer1 = factory::CreateSpacer(0, 10);
    helper::addChild(m_registry, m_rootEntity, spacer1);

    // 用户名标签
    auto usernameLabel = factory::CreateLabel("Username");
    auto& usernameLabelText = m_registry.get<components::Text>(usernameLabel);
    usernameLabelText.color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
    helper::addChild(m_registry, m_rootEntity, usernameLabel);

    // 用户名输入框
    m_usernameInput = factory::CreateTextInput("");
    helper::setFixedSize(m_registry, m_usernameInput, 340, 35);
    helper::setBackgroundColor(m_registry, m_usernameInput, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
    helper::setBorderRadius(m_registry, m_usernameInput, 6.0f);
    helper::addChild(m_registry, m_rootEntity, m_usernameInput);

    // 密码标签
    auto passwordLabel = factory::CreateLabel("Password");
    auto& passwordLabelText = m_registry.get<components::Text>(passwordLabel);
    passwordLabelText.color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
    helper::addChild(m_registry, m_rootEntity, passwordLabel);

    // 密码输入框
    m_passwordInput = factory::CreateTextInput("");
    helper::setFixedSize(m_registry, m_passwordInput, 340, 35);
    helper::setPasswordMode(m_registry, m_passwordInput, true);
    helper::setBackgroundColor(m_registry, m_passwordInput, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
    helper::setBorderRadius(m_registry, m_passwordInput, 6.0f);
    helper::addChild(m_registry, m_rootEntity, m_passwordInput);

    // 间隔
    auto spacer2 = factory::CreateSpacer(0, 15);
    helper::addChild(m_registry, m_rootEntity, spacer2);

    // 按钮容器
    auto buttonBox = factory::CreateHBox();
    helper::setSpacing(m_registry, buttonBox, 10);

    // 登录按钮
    auto loginBtn = factory::CreateButton("Login");
    helper::setFixedSize(m_registry, loginBtn, 165, 40);
    helper::setBackgroundColor(m_registry, loginBtn, ImVec4(0.2f, 0.6f, 0.3f, 1.0f));
    helper::setBorderRadius(m_registry, loginBtn, 6.0f);

    auto& loginClickable = m_registry.get<components::Clickable>(loginBtn);
    loginClickable.onClick = [this](entt::entity) { onLoginButtonClicked(); };

    // 退出按钮
    auto quitBtn = factory::CreateButton("Quit");
    helper::setFixedSize(m_registry, quitBtn, 165, 40);
    helper::setBackgroundColor(m_registry, quitBtn, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
    helper::setBorderRadius(m_registry, quitBtn, 6.0f);

    auto& quitClickable = m_registry.get<components::Clickable>(quitBtn);
    quitClickable.onClick = [this](entt::entity) { onQuitButtonClicked(); };

    helper::addChild(m_registry, buttonBox, loginBtn);
    helper::addChild(m_registry, buttonBox, quitBtn);
    helper::addChild(m_registry, m_rootEntity, buttonBox);

    // 居中显示
    helper::centerInParent(m_registry, m_rootEntity);

    // 淡入动画
    factory::CreateFadeInAnimation(m_rootEntity, 0.4f);
}

void LoginScene::onLoginButtonClicked()
{
    if (!m_client) return;

    // 获取输入内容
    const auto& username = m_registry.get<components::TextEdit>(m_usernameInput).buffer;
    const auto& password = m_registry.get<components::TextEdit>(m_passwordInput).buffer;

    // 验证输入
    if (username.empty())
    {
        // TODO: 显示错误提示 "请输入用户名"
        return;
    }

    if (password.empty())
    {
        // TODO: 显示错误提示 "请输入密码"
        return;
    }

    // 执行登录（这里简化处理，实际应该发送登录请求到服务器）
    if (m_client->isConnected())
    {
        // 切换到主菜单
        SceneManager::getInstance().switchTo("MainMenu");
    }
    else
    {
        // TODO: 显示 "未连接到服务器"
    }
}

void LoginScene::onQuitButtonClicked()
{
    if (m_client)
    {
        m_client->stop();
    }
    // TODO: 退出应用程序
}

// ========== MainMenuScene 实现 ==========

void MainMenuScene::enter()
{
    createUI();
}

void MainMenuScene::exit()
{
    if (m_registry.valid(m_rootEntity))
    {
        helper::traverseChildren(m_registry,
                                 m_rootEntity,
                                 [this](entt::entity entity)
                                 {
                                     if (m_registry.valid(entity))
                                     {
                                         m_registry.destroy(entity);
                                     }
                                 });
        m_registry.destroy(m_rootEntity);
        m_rootEntity = entt::null;
    }
}

void MainMenuScene::update(float deltaTime)
{
    // 主菜单更新逻辑
}

void MainMenuScene::createUI()
{
    // 创建主容器
    m_rootEntity = factory::CreateVBox();
    helper::setFixedSize(m_registry, m_rootEntity, 500, 450);
    helper::setPadding(m_registry, m_rootEntity, 25);
    helper::setBackgroundColor(m_registry, m_rootEntity, ImVec4(0.1f, 0.1f, 0.12f, 0.95f));
    helper::setBorderRadius(m_registry, m_rootEntity, 12.0f);
    helper::setSpacing(m_registry, m_rootEntity, 15.0f);

    // 标题
    auto title = factory::CreateLabel("Main Menu");
    auto& titleText = m_registry.get<components::Text>(title);
    titleText.fontSize = 26.0f;
    titleText.alignment = components::Alignment::CENTER;
    helper::addChild(m_registry, m_rootEntity, title);

    // 玩家信息
    if (m_client)
    {
        auto playerInfo = factory::CreateLabel("Player: " + m_client->getPlayerName());
        auto& infoText = m_registry.get<components::Text>(playerInfo);
        infoText.fontSize = 14.0f;
        infoText.color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
        infoText.alignment = components::Alignment::CENTER;
        helper::addChild(m_registry, m_rootEntity, playerInfo);
    }

    // 间隔
    auto spacer = factory::CreateSpacer(0, 20);
    helper::addChild(m_registry, m_rootEntity, spacer);

    // 创建房间按钮
    auto createRoomBtn = factory::CreateButton("Create Room");
    helper::setFixedSize(m_registry, createRoomBtn, 450, 50);
    helper::setBackgroundColor(m_registry, createRoomBtn, ImVec4(0.25f, 0.5f, 0.7f, 1.0f));
    helper::setBorderRadius(m_registry, createRoomBtn, 8.0f);
    auto& createClickable = m_registry.get<components::Clickable>(createRoomBtn);
    createClickable.onClick = [this](entt::entity) { onCreateRoomClicked(); };
    helper::addChild(m_registry, m_rootEntity, createRoomBtn);

    // 加入房间按钮
    auto joinRoomBtn = factory::CreateButton("Join Room");
    helper::setFixedSize(m_registry, joinRoomBtn, 450, 50);
    helper::setBackgroundColor(m_registry, joinRoomBtn, ImVec4(0.3f, 0.6f, 0.3f, 1.0f));
    helper::setBorderRadius(m_registry, joinRoomBtn, 8.0f);
    auto& joinClickable = m_registry.get<components::Clickable>(joinRoomBtn);
    joinClickable.onClick = [this](entt::entity) { onJoinRoomClicked(); };
    helper::addChild(m_registry, m_rootEntity, joinRoomBtn);

    // 设置按钮
    auto settingsBtn = factory::CreateButton("Settings");
    helper::setFixedSize(m_registry, settingsBtn, 450, 50);
    helper::setBackgroundColor(m_registry, settingsBtn, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    helper::setBorderRadius(m_registry, settingsBtn, 8.0f);
    auto& settingsClickable = m_registry.get<components::Clickable>(settingsBtn);
    settingsClickable.onClick = [this](entt::entity) { onSettingsClicked(); };
    helper::addChild(m_registry, m_rootEntity, settingsBtn);

    // 登出按钮
    auto logoutBtn = factory::CreateButton("Logout");
    helper::setFixedSize(m_registry, logoutBtn, 450, 50);
    helper::setBackgroundColor(m_registry, logoutBtn, ImVec4(0.7f, 0.3f, 0.3f, 1.0f));
    helper::setBorderRadius(m_registry, logoutBtn, 8.0f);
    auto& logoutClickable = m_registry.get<components::Clickable>(logoutBtn);
    logoutClickable.onClick = [this](entt::entity) { onLogoutClicked(); };
    helper::addChild(m_registry, m_rootEntity, logoutBtn);

    // 居中
    helper::centerInParent(m_registry, m_rootEntity);

    // 淡入动画
    factory::CreateFadeInAnimation(m_rootEntity, 0.3f);
}

void MainMenuScene::onCreateRoomClicked()
{
    if (!m_client || !m_client->isConnected()) return;

    // 发送创建房间请求
    auto req = CreateRoomRequest::create("My Room", 8, 0);
    m_client->sendMessage(req);
}

void MainMenuScene::onJoinRoomClicked()
{
    // TODO: 显示房间列表对话框
}

void MainMenuScene::onSettingsClicked()
{
    // TODO: 显示设置对话框
}

void MainMenuScene::onLogoutClicked()
{
    SceneManager::getInstance().switchTo("Login");
}

// ========== RoomScene 和 GameScene 简化实现 ==========

void RoomScene::enter()
{
    createUI();
}
void RoomScene::exit()
{
    if (m_registry.valid(m_rootEntity))
    {
        m_registry.destroy(m_rootEntity);
    }
}
void RoomScene::update(float) {}
void RoomScene::createUI()
{
    // TODO: 实现房间 UI
}
void RoomScene::onStartGameClicked() {}
void RoomScene::onLeaveRoomClicked()
{
    SceneManager::getInstance().switchTo("MainMenu");
}
void RoomScene::updatePlayerList() {}

void GameScene::enter()
{
    createUI();
}
void GameScene::exit()
{
    if (m_registry.valid(m_rootEntity))
    {
        m_registry.destroy(m_rootEntity);
    }
}
void GameScene::update(float) {}
void GameScene::createUI()
{
    // TODO: 实现游戏 UI
}
void GameScene::updateGameState() {}
