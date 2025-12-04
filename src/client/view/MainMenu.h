/**
 * ************************************************************************
 *
 * @file MainMenu.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-03
 * @version 0.1
 * @brief 主菜单视图
    ListArea 纵向排列
    三个按钮： 创建房间， 加入房间，退出游戏
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once

#include <memory>
#include <functional>
#include "src/client/ui/Widget.h"
#include "src/client/ui/Button.h"
#include "src/client/ui/Label.h"
#include "src/client/ui/ListArea.h"
#include "src/client/ui/Layout.h"
#include "src/client/ui/Spacer.h"

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

/**
 * @brief 主菜单界面
 * 提供创建房间、加入房间、退出游戏等功能
 */
class MainMenu : public ui::Widget
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

    explicit MainMenu(Callbacks callbacks = {}) : m_callbacks(std::move(callbacks)) { setupUI(); }

    ~MainMenu() override = default;
    MainMenu(const MainMenu&) = delete;
    MainMenu& operator=(const MainMenu&) = delete;
    MainMenu(MainMenu&&) = delete;
    MainMenu& operator=(MainMenu&&) = delete;

    /**
     * @brief 设置回调
     */
    void setCallbacks(const Callbacks& callbacks) { m_callbacks = callbacks; }

    /**
     * @brief 更新标题文本
     */
    void setTitle(const std::string& title)
    {
        if (m_titleLabel)
        {
            m_titleLabel = std::make_shared<ui::Label>(title);
            setupUI(); // 重新构建 UI
        }
    }

protected:
    void onRender(const ImVec2& position, const ImVec2& size) override
    {
        if (m_layout)
        {
            m_layout->render(position, size);
        }
    }

private:
    void setupUI()
    {
        // 创建主布局（垂直）
        m_layout = std::make_shared<ui::VBoxLayout>();
        m_layout->setBackgroundEnabled(true);
        m_layout->setBackgroundColor(ImVec4(0.1F, 0.12F, 0.15F, 0.95F)); // 深色半透明背景
        m_layout->setSpacing(20);
        m_layout->setMargins(40, 40, 40, 40);

        // 添加顶部弹性空间
        m_layout->addStretch(1);

        // 游戏标题
        m_titleLabel = std::make_shared<ui::Label>("PestManKill");
        m_titleLabel->setFixedSize(400, 80);
        m_layout->addWidget(m_titleLabel);

        // 添加间距
        m_layout->addWidget(std::make_shared<ui::Spacer>(0, 20));

        // 创建按钮区域
        auto buttonArea = createButtonArea();
        m_layout->addWidget(buttonArea);

        // 添加底部弹性空间
        m_layout->addStretch(1);

        // 版本信息
        auto versionLabel = std::make_shared<ui::Label>("Version 0.1 - Learning Project");
        m_layout->addWidget(versionLabel);

        // 将布局添加为子组件
        addChild(m_layout);
    }

    std::shared_ptr<ui::Widget> createButtonArea()
    {
        // 创建按钮列表区域
        auto buttonList = std::make_shared<ui::ListArea>(ui::ListDirection::VERTICAL);
        buttonList->setFixedSize(300, 0);

        // 创建房间按钮
        auto createRoomBtn = std::make_shared<ui::Button>("创建房间",
                                                          [this]()
                                                          {
                                                              if (m_callbacks.onCreateRoom)
                                                              {
                                                                  m_callbacks.onCreateRoom();
                                                              }
                                                          });
        createRoomBtn->setFixedSize(300, 50);
        buttonList->addWidget(createRoomBtn);

        // 加入房间按钮
        auto joinRoomBtn = std::make_shared<ui::Button>("加入房间",
                                                        [this]()
                                                        {
                                                            if (m_callbacks.onJoinRoom)
                                                            {
                                                                m_callbacks.onJoinRoom();
                                                            }
                                                        });
        joinRoomBtn->setFixedSize(300, 50);
        buttonList->addWidget(joinRoomBtn);

        // 退出游戏按钮
        auto exitGameBtn = std::make_shared<ui::Button>("退出游戏",
                                                        [this]()
                                                        {
                                                            if (m_callbacks.onExitGame)
                                                            {
                                                                m_callbacks.onExitGame();
                                                            }
                                                        });
        exitGameBtn->setFixedSize(300, 50);
        buttonList->addWidget(exitGameBtn);

        return buttonList;
    }

    Callbacks m_callbacks;
    std::shared_ptr<ui::VBoxLayout> m_layout;
    std::shared_ptr<ui::Label> m_titleLabel;
};

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
