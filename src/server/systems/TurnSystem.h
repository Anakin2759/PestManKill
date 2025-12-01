/**
 * ************************************************************************
 *
 * @file TurnSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2025-11-25
 * @version 0.1
 * @brief 回合系统定义
   处理游戏回合流程

 *
 * ************************************************************************
 * @copyright Copyright (c) 2025 AnakinLiu
 * For study and research only, no reprinting.
 * ************************************************************************
 */

#pragma once
#include <entt/entt.hpp>
#include "src/server/context/GameContext.h"
#include "src/server/components/Events.h"

class TurnSystem
{
public:
    explicit TurnSystem(GameContext& context)
        : m_context(&context) {

          };
    void registerEvents()
    {
        m_context->dispatcher.sink<events::TurnPhase>().connect<&TurnSystem::onTurnPhaseChanged>(this);
    };
    void unregisterEvents()
    {
        m_context->dispatcher.sink<events::TurnPhase>().disconnect<&TurnSystem::onTurnPhaseChanged>(this);
    };

private:
    void onTurnPhaseChanged(const events::TurnPhase& event) const
    {
        // 处理回合阶段变化的逻辑
        switch (event.currentPhase)
        {
            case TurnPhase::GAME_START:
            {
                m_context->logger->info("游戏开始");
                break;
            }
            case TurnPhase::START:
            {
                // 处理开始阶段
                m_context->logger->info("回合开始阶段");
                break;
            }
            case TurnPhase::JUDGE:
            {
                // 处理判定阶段
                m_context->logger->info("判定阶段");
                break;
            }
            case TurnPhase::DRAW:
            {
                // 处理摸牌阶段
                m_context->logger->info("摸牌阶段");
                break;
            }
            case TurnPhase::PLAY:
                // 处理出牌阶段
                m_context->logger->info("出牌阶段");
                break;
            case TurnPhase::DISCARD:
                // 处理弃牌阶段
                m_context->logger->info("弃牌阶段");
                break;
            case TurnPhase::END:
                // 处理结束阶段
                m_context->logger->info("结束阶段");
                break;
            case TurnPhase::GAME_OVER:
                // 处理游戏结束阶段
                m_context->logger->info("游戏结束");
                break;
            default:
                break;
        }
    };

    void onProceedToNextPhase() {
        // 实现回合阶段的推进逻辑
    };

    GameContext* m_context;
};
