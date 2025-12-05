/**
 * ************************************************************************
 *
 * @file MainMenuECS.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-05
 * @version 0.1
 * @brief 主菜单视图 - ECS版本
 * 提供创建房间、加入房间、退出游戏等功能
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include "src/client/ui/ECSApplication.h"
#include "src/client/model/UIFactory.h"
#include "src/client/model/UIHelper.h"
#include "src/client/events/UIEvents.h"
#include "src/shared/utils/Dispatcher.h"
#include <functional>
#include <string>

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

/**
 * @brief 主菜单界面 - ECS版本
 * 深色背景，垂直布局，包含标题和三个操作按钮
 */
class MainMenuECS : public ui::ECSApplication
{
public:
    /**
     * @brief 菜单操作回调
     */
    struct Callbacks
    {
        std::function<void()> onCreateRoom; // 创建房间回调
        std::function<void()> onJoinRoom;   // 加入房间回调
        std::function<void()> onExitGame;   // 退出游戏回调
    };

    explicit MainMenuECS(const std::string& title = "PestManKill", Callbacks callbacks = {})
        : ECSApplication(title, 800, 600), m_callbacks(std::move(callbacks))
    {
    }

    ~MainMenuECS() override = default;
    MainMenuECS(const MainMenuECS&) = delete;
    MainMenuECS& operator=(const MainMenuECS&) = delete;
    MainMenuECS(MainMenuECS&&) = delete;
    MainMenuECS& operator=(MainMenuECS&&) = delete;

    /**
     * @brief 设置回调
     */
    void setCallbacks(const Callbacks& callbacks) { m_callbacks = callbacks; }

    /**
     * @brief 获取主容器实体
     */
    [[nodiscard]] entt::entity getMainContainer() const { return m_mainContainer; }

protected:
    void setupUI() override
    {
        auto& registry = m_uiSystem->getRegistry();
        auto& factory = m_uiSystem->getFactory();

        // 创建主容器 - 深色半透明背景
        m_mainContainer = factory.createContainer(
            {0.0F, 0.0F}, {800.0F, 600.0F}, ImVec4(0.1F, 0.12F, 0.15F, 0.95F) // 深色背景，匹配老版本
        );

        // 添加垂直布局
        registry.emplace<ui::VerticalLayout>(m_mainContainer, 20.0F);               // spacing 20
        registry.emplace<ui::Padding>(m_mainContainer, 40.0F, 40.0F, 40.0F, 40.0F); // margins 40

        // 顶部弹性空间
        auto topSpacer = factory.createSpacer({0.0F, 0.0F}, {0.0F, 0.0F});
        ui::UIHelper::addChild(registry, m_mainContainer, topSpacer, 1.0F);

        // 游戏标题
        m_titleLabel =
            factory.createLabel({0.0F, 0.0F}, {400.0F, 80.0F}, "PestManKill", 24.0F, ImVec4(1.0F, 1.0F, 1.0F, 1.0F));
        ui::UIHelper::addChild(registry, m_mainContainer, m_titleLabel);

        // 间距
        auto spacer1 = factory.createSpacer({0.0F, 0.0F}, {0.0F, 20.0F});
        ui::UIHelper::addChild(registry, m_mainContainer, spacer1);

        // 按钮区域容器
        auto buttonContainer =
            factory.createContainer({0.0F, 0.0F}, {300.0F, 0.0F}, ImVec4(0.0F, 0.0F, 0.0F, 0.0F) // 透明背景
            );
        registry.emplace<ui::VerticalLayout>(buttonContainer, 10.0F);
        ui::UIHelper::addChild(registry, m_mainContainer, buttonContainer);

        // 创建房间按钮
        m_createRoomBtn =
            factory.createButton({0.0F, 0.0F}, {300.0F, 50.0F}, "创建房间", [this]() { onCreateRoomClicked(); });
        ui::UIHelper::addChild(registry, buttonContainer, m_createRoomBtn);

        // 加入房间按钮
        m_joinRoomBtn =
            factory.createButton({0.0F, 0.0F}, {300.0F, 50.0F}, "加入房间", [this]() { onJoinRoomClicked(); });
        ui::UIHelper::addChild(registry, buttonContainer, m_joinRoomBtn);

        // 退出游戏按钮
        m_exitGameBtn =
            factory.createButton({0.0F, 0.0F}, {300.0F, 50.0F}, "退出游戏", [this]() { onExitGameClicked(); });
        ui::UIHelper::addChild(registry, buttonContainer, m_exitGameBtn);

        // 底部弹性空间
        auto bottomSpacer = factory.createSpacer({0.0F, 0.0F}, {0.0F, 0.0F});
        ui::UIHelper::addChild(registry, m_mainContainer, bottomSpacer, 1.0F);

        // 版本信息
        auto versionLabel = factory.createLabel(
            {0.0F, 0.0F}, {0.0F, 0.0F}, "Version 0.1 - Learning Project", 12.0F, ImVec4(0.7F, 0.7F, 0.7F, 1.0F));
        ui::UIHelper::addChild(registry, m_mainContainer, versionLabel);

        // 设置根实体
        m_uiSystem->setRootEntity(m_mainContainer);
    }

private:
    void onCreateRoomClicked()
    {
        if (m_callbacks.onCreateRoom)
        {
            m_callbacks.onCreateRoom();
        }

        // 触发事件
        utils::Dispatcher::getInstance().trigger<ui::events::ButtonClickEvent>(
            ui::events::ButtonClickEvent{m_createRoomBtn, "create_room"});
    }

    void onJoinRoomClicked()
    {
        if (m_callbacks.onJoinRoom)
        {
            m_callbacks.onJoinRoom();
        }

        utils::Dispatcher::getInstance().trigger<ui::events::ButtonClickEvent>(
            ui::events::ButtonClickEvent{m_joinRoomBtn, "join_room"});
    }

    void onExitGameClicked()
    {
        if (m_callbacks.onExitGame)
        {
            m_callbacks.onExitGame();
        }

        utils::Dispatcher::getInstance().trigger<ui::events::ButtonClickEvent>(
            ui::events::ButtonClickEvent{m_exitGameBtn, "exit_game"});

        // 退出应用
        m_running = false;
    }

    Callbacks m_callbacks;
    entt::entity m_mainContainer{entt::null};
    entt::entity m_titleLabel{entt::null};
    entt::entity m_createRoomBtn{entt::null};
    entt::entity m_joinRoomBtn{entt::null};
    entt::entity m_exitGameBtn{entt::null};
};

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
