/**
 * ************************************************************************
 *
 * @file SceneManager.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-18
 * @version 0.1
 * @brief UI 场景管理器
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <utils.h>

class GameClient;

/**
 * @brief 场景基类
 */
class Scene
{
public:
    virtual ~Scene() = default;

    virtual void enter() = 0;                 // 进入场景
    virtual void exit() = 0;                  // 退出场景
    virtual void update(float deltaTime) = 0; // 更新场景

    void setClient(GameClient* client) { m_client = client; }

protected:
    GameClient* m_client = nullptr;
    entt::registry& m_registry = utils::Registry::getInstance();
};

/**
 * @brief 登录场景
 */
class LoginScene : public Scene
{
public:
    void enter() override;
    void exit() override;
    void update(float deltaTime) override;

private:
    void createUI();
    void onLoginButtonClicked();
    void onQuitButtonClicked();

    entt::entity m_rootEntity = entt::null;
    entt::entity m_usernameInput = entt::null;
    entt::entity m_passwordInput = entt::null;
};

/**
 * @brief 主菜单场景
 */
class MainMenuScene : public Scene
{
public:
    void enter() override;
    void exit() override;
    void update(float deltaTime) override;

private:
    void createUI();
    void onCreateRoomClicked();
    void onJoinRoomClicked();
    void onSettingsClicked();
    void onLogoutClicked();

    entt::entity m_rootEntity = entt::null;
};

/**
 * @brief 房间场景
 */
class RoomScene : public Scene
{
public:
    void enter() override;
    void exit() override;
    void update(float deltaTime) override;

    void setRoomId(uint32_t roomId) { m_roomId = roomId; }

private:
    void createUI();
    void onStartGameClicked();
    void onLeaveRoomClicked();
    void updatePlayerList();

    uint32_t m_roomId = 0;
    entt::entity m_rootEntity = entt::null;
    entt::entity m_playerListContainer = entt::null;
};

/**
 * @brief 游戏场景
 */
class GameScene : public Scene
{
public:
    void enter() override;
    void exit() override;
    void update(float deltaTime) override;

private:
    void createUI();
    void updateGameState();

    entt::entity m_rootEntity = entt::null;
};

/**
 * @brief 场景管理器
 */
class SceneManager
{
public:
    static SceneManager& getInstance()
    {
        static SceneManager instance;
        return instance;
    }

    void setClient(GameClient* client) { m_client = client; }

    /**
     * @brief 注册场景
     */
    template <typename SceneType>
    void registerScene(const std::string& name)
    {
        m_sceneFactories[name] = []() -> std::unique_ptr<Scene> { return std::make_unique<SceneType>(); };
    }

    /**
     * @brief 切换场景
     */
    void switchTo(const std::string& sceneName)
    {
        auto it = m_sceneFactories.find(sceneName);
        if (it == m_sceneFactories.end())
        {
            LOG_WARN("Scene not found: {}", sceneName);
            return;
        }

        // 退出当前场景
        if (m_currentScene)
        {
            m_currentScene->exit();
        }

        // 创建并进入新场景
        m_currentScene = it->second();
        if (m_client)
        {
            m_currentScene->setClient(m_client);
        }
        m_currentScene->enter();

        m_currentSceneName = sceneName;
        m_logger->info("Switched to scene: {}", sceneName);
    }

    /**
     * @brief 更新当前场景
     */
    void update(float deltaTime)
    {
        if (m_currentScene)
        {
            m_currentScene->update(deltaTime);
        }
    }

    /**
     * @brief 获取当前场景名称
     */
    const std::string& getCurrentSceneName() const { return m_currentSceneName; }

private:
    SceneManager()
    {
        m_logger = spdlog::stdout_color_mt("SceneManager");

        // 注册默认场景
        registerScene<LoginScene>("Login");
        registerScene<MainMenuScene>("MainMenu");
        registerScene<RoomScene>("Room");
        registerScene<GameScene>("Game");
    }

    GameClient* m_client = nullptr;
    std::unique_ptr<Scene> m_currentScene;
    std::string m_currentSceneName;
    std::unordered_map<std::string, std::function<std::unique_ptr<Scene>()>> m_sceneFactories;
    std::shared_ptr<spdlog::logger> m_logger;
};
