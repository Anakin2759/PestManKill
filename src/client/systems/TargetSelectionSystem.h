/**
 * ************************************************************************
 *
 * @file TargetSelectionSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-12-09
 * @version 0.1
 * @brief 目标选择系统
 *
 * 管理卡牌使用时的目标选择流程
 * - 处理目标选择开始/结束
 * - 验证目标有效性
 * - 管理选中的目标列表
 * - 发送使用卡牌请求
 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include <vector>
#include <optional>
#include <memory>
#include "src/client/events/TargetSelectionEvents.h"
#include "src/client/net/GameClient.h"
#include "src/client/utils/Logger.h"
#include "src/client/utils/Dispatcher.h"

namespace systems
{

class TargetSelectionSystem
{
public:
    explicit TargetSelectionSystem(std::shared_ptr<GameClient> gameClient) : m_gameClient(std::move(gameClient))
    {
        registerEvents();
    }

    ~TargetSelectionSystem() { unregisterEvents(); }

    void registerEvents()
    {
        auto& dispatcher = utils::Dispatcher::getInstance();
        dispatcher.sink<events::StartTargetSelection>().connect<&TargetSelectionSystem::onStartTargetSelection>(this);
        dispatcher.sink<events::TargetSelected>().connect<&TargetSelectionSystem::onTargetSelected>(this);
        dispatcher.sink<events::TargetDeselected>().connect<&TargetSelectionSystem::onTargetDeselected>(this);
        dispatcher.sink<events::ConfirmTargetSelection>().connect<&TargetSelectionSystem::onConfirmTargetSelection>(
            this);
        dispatcher.sink<events::CancelTargetSelection>().connect<&TargetSelectionSystem::onCancelTargetSelection>(this);
    }

    void unregisterEvents()
    {
        auto& dispatcher = utils::Dispatcher::getInstance();
        dispatcher.sink<events::StartTargetSelection>().disconnect<&TargetSelectionSystem::onStartTargetSelection>(
            this);
        dispatcher.sink<events::TargetSelected>().disconnect<&TargetSelectionSystem::onTargetSelected>(this);
        dispatcher.sink<events::TargetDeselected>().disconnect<&TargetSelectionSystem::onTargetDeselected>(this);
        dispatcher.sink<events::ConfirmTargetSelection>().disconnect<&TargetSelectionSystem::onConfirmTargetSelection>(
            this);
        dispatcher.sink<events::CancelTargetSelection>().disconnect<&TargetSelectionSystem::onCancelTargetSelection>(
            this);
    }

    [[nodiscard]] bool isSelecting() const { return m_isSelecting; }

    [[nodiscard]] const std::vector<entt::entity>& getSelectedTargets() const { return m_selectedTargets; }

    [[nodiscard]] entt::entity getCurrentCard() const { return m_currentCard; }

private:
    void onStartTargetSelection(const events::StartTargetSelection& event)
    {
        if (m_isSelecting)
        {
            LOG_WARN("Already selecting targets, canceling previous selection");
            clearSelection();
        }

        m_isSelecting = true;
        m_currentCard = event.cardEntity;
        m_minTargets = event.minTargets;
        m_maxTargets = event.maxTargets;
        m_range = event.range;
        m_selectedTargets.clear();

        LOG_INFO("Started target selection for card: entity={}, min={}, max={}, range={}",
                 static_cast<uint32_t>(event.cardEntity),
                 event.minTargets,
                 event.maxTargets,
                 event.range);
    }

    void onTargetSelected(const events::TargetSelected& event)
    {
        if (!m_isSelecting)
        {
            LOG_WARN("Not in target selection mode");
            return;
        }

        // 检查是否已选中
        auto iter = std::find(m_selectedTargets.begin(), m_selectedTargets.end(), event.targetEntity);
        if (iter != m_selectedTargets.end())
        {
            LOG_WARN("Target already selected: entity={}", static_cast<uint32_t>(event.targetEntity));
            return;
        }

        // 检查是否超过最大数量
        if (m_selectedTargets.size() >= m_maxTargets)
        {
            LOG_WARN("Maximum number of targets reached: {}", m_maxTargets);
            return;
        }

        // TODO: 添加距离和有效性验证

        m_selectedTargets.push_back(event.targetEntity);
        LOG_INFO("Target selected: entity={}, count={}/{}",
                 static_cast<uint32_t>(event.targetEntity),
                 m_selectedTargets.size(),
                 m_maxTargets);
    }

    void onTargetDeselected(const events::TargetDeselected& event)
    {
        if (!m_isSelecting)
        {
            LOG_WARN("Not in target selection mode");
            return;
        }

        auto iter = std::find(m_selectedTargets.begin(), m_selectedTargets.end(), event.targetEntity);
        if (iter == m_selectedTargets.end())
        {
            LOG_WARN("Target not found in selection: entity={}", static_cast<uint32_t>(event.targetEntity));
            return;
        }

        m_selectedTargets.erase(iter);
        LOG_INFO("Target deselected: entity={}, count={}/{}",
                 static_cast<uint32_t>(event.targetEntity),
                 m_selectedTargets.size(),
                 m_maxTargets);
    }

    void onConfirmTargetSelection(const events::ConfirmTargetSelection& event)
    {
        if (!m_isSelecting)
        {
            LOG_WARN("Not in target selection mode");
            return;
        }

        // 验证选择的目标数量
        if (m_selectedTargets.size() < m_minTargets)
        {
            LOG_WARN("Not enough targets selected: {}/{}", m_selectedTargets.size(), m_minTargets);
            return;
        }

        if (m_selectedTargets.size() > m_maxTargets)
        {
            LOG_WARN("Too many targets selected: {}/{}", m_selectedTargets.size(), m_maxTargets);
            return;
        }

        // 转换实体ID为uint32_t
        std::vector<uint32_t> targetIds;
        targetIds.reserve(m_selectedTargets.size());
        for (auto target : m_selectedTargets)
        {
            targetIds.push_back(static_cast<uint32_t>(target));
        }

        uint32_t cardId = static_cast<uint32_t>(event.cardEntity);

        LOG_INFO("Confirming target selection: card={}, targets={}", cardId, targetIds.size());

        // 发送使用卡牌请求
        if (m_gameClient)
        {
            asio::co_spawn(
                utils::ThreadPool::getInstance().get_executor(),
                [client = m_gameClient, cardId, targetIds]() -> asio::awaitable<void>
                {
                    bool success = co_await client->sendUseCard(cardId, targetIds);
                    if (success)
                    {
                        LOG_INFO("Use card request sent successfully");
                        // 触发请求发送事件
                        utils::Dispatcher::getInstance().enqueue<events::UseCardRequestSent>(
                            events::UseCardRequestSent{cardId, targetIds});
                    }
                    else
                    {
                        LOG_ERROR("Failed to send use card request");
                    }
                },
                asio::detached);
        }
        else
        {
            LOG_ERROR("GameClient is not available");
        }

        clearSelection();
    }

    void onCancelTargetSelection(const events::CancelTargetSelection& event)
    {
        if (!m_isSelecting)
        {
            LOG_WARN("Not in target selection mode");
            return;
        }

        LOG_INFO("Target selection canceled for card: entity={}", static_cast<uint32_t>(event.cardEntity));
        clearSelection();
    }

    void clearSelection()
    {
        m_isSelecting = false;
        m_currentCard = entt::null;
        m_selectedTargets.clear();
        m_minTargets = 0;
        m_maxTargets = 0;
        m_range = 0;
    }

private:
    std::shared_ptr<GameClient> m_gameClient;
    bool m_isSelecting = false;
    entt::entity m_currentCard = entt::null;
    std::vector<entt::entity> m_selectedTargets;
    uint32_t m_minTargets = 0;
    uint32_t m_maxTargets = 0;
    uint32_t m_range = 0;
};

} // namespace systems
