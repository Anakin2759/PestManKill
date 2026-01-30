/**
 * ************************************************************************
 *
 * @file InteractionSystem.h
 * @author AnakinLiu (azrael2759@qq.com)
 * @date 2026-01-28 (Refactored)
 * @version 0.3
 * @brief 交互处理系统 - SDL事件捕获与分发层
 *
 * 职责：
 * 1. 捕获SDL原始事件（鼠标、键盘、滚轮、窗口）
 * 2. 将事件转换为内部事件并分发到事件总线
 * 3. 调用HitTestSystem进行碰撞检测
 * 4. 触发StateSystem和ActionSystem处理后续逻辑
 *
 * 事件链条：
 * SDL事件捕获（InteractionSystem）
 *   ├─→ 鼠标/滚轮事件 → HitTestSystem碰撞检测 → 触发Hover/Press/Release事件
 *   │                                              ↓
 *   │                                   StateSystem状态管理（Hover/Active/Focus）
 *   │                                              ↓
 *   │                                   ActionSystem执行回调
 *   │
 *   ├─→ 键盘事件 → 文本输入/按键处理 → 更新TextEdit组件
 *   │
 *   └─→ 窗口事件（DetailExposed监听）→ StateSystem窗口同步 → RenderSystem渲染更新
 *
 * ************************************************************************
 */
#pragma once
#include <entt/entt.hpp>
#include <algorithm>
#include <string>
#include <utility>
#include <SDL3/SDL.h>
#include "common/Events.hpp"
#include "singleton/Registry.hpp"
#include "singleton/Dispatcher.hpp"
#include "common/Components.hpp"
#include "common/Tags.hpp"
#include "api/Utils.hpp"
#include "interface/Isystem.hpp"
#include "common/Types.hpp"
#include "HitTestSystem.hpp"
#include "StateSystem.hpp"

namespace ui::systems
{

class InteractionSystem : public ui::interface::EnableRegister<InteractionSystem>
{
public:
    void registerHandlersImpl() { DetailExposed(); }

    void unregisterHandlersImpl() {}
    /**
     * @brief 处理 SDL每tick事件
     *
     * - 负责 SDL_PollEvent 事件
     * - 识别 Quit / Window Resized，并通过回调交由上层处理
     * - 直接从 SDL 事件中追踪鼠标状态
     */
    static void SDLEvent()
    {
        SDL_Event event{};

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                    Dispatcher::Enqueue<ui::events::QuitRequested>();
                    break;

                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                {
                    // 查找对应的 Window 实体
                    entt::entity targetWindow = entt::null;
                    auto view = Registry::View<components::Window>();
                    for (auto entity : view)
                    {
                        if (view.get<components::Window>(entity).windowID == event.window.windowID)
                        {
                            targetWindow = entity;
                            break;
                        }
                    }

                    if (Registry::Valid(targetWindow))
                    {
                        Dispatcher::Enqueue<ui::events::CloseWindow>(ui::events::CloseWindow{targetWindow});
                    }
                    break;
                }

                case SDL_EVENT_MOUSE_MOTION:
                {
                    // 不在此处进行命中/滚动处理：只记录并转发原始指针移动数据
                    {
                        float mx = static_cast<float>(event.motion.x);
                        float my = static_cast<float>(event.motion.y);
                        float dx = static_cast<float>(event.motion.xrel);
                        float dy = static_cast<float>(event.motion.yrel);
                        uint32_t winId = event.motion.windowID;
                        Dispatcher::Enqueue<ui::events::RawPointerMove>(
                            ui::events::RawPointerMove{Vec2(mx, my), Vec2(dx, dy), winId});
                    }
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                {
                    float mx = static_cast<float>(event.button.x);
                    float my = static_cast<float>(event.button.y);
                    uint32_t winId = event.button.windowID;
                    Dispatcher::Enqueue<ui::events::RawPointerButton>(
                        ui::events::RawPointerButton{Vec2(mx, my), winId, true});

                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_UP:
                {
                    float mx = static_cast<float>(event.button.x);
                    float my = static_cast<float>(event.button.y);
                    uint32_t winId = event.button.windowID;
                    Dispatcher::Enqueue<ui::events::RawPointerButton>(
                        ui::events::RawPointerButton{Vec2(mx, my), winId, false});

                    break;
                }
                case SDL_EVENT_TEXT_INPUT:
                    handleTextInput(event.text.text);

                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (!event.key.repeat) // 只处理真实按下，忽略系统重复
                    {
                        m_heldKey = event.key.key;
                        m_keyPressTime = SDL_GetTicks();
                        m_lastRepeatTime = m_keyPressTime;
                        handleKeyDown(event.key.key);
                    }

                    break;
                case SDL_EVENT_KEY_UP:
                    // 重置长按状态
                    if (event.key.key == m_heldKey)
                    {
                        m_heldKey = SDLK_UNKNOWN;
                        m_keyPressTime = 0;
                        m_lastRepeatTime = 0;
                    }
                    break;
                case SDL_EVENT_MOUSE_WHEEL:
                {
                    // 采样当前鼠标位置并转发原始滚轮事件
                    float mx = 0.0f, my = 0.0f;
                    SDL_GetMouseState(&mx, &my);
                    Dispatcher::Enqueue<ui::events::RawPointerWheel>(ui::events::RawPointerWheel{
                        Vec2(mx, my),
                        Vec2(static_cast<float>(event.wheel.x), static_cast<float>(event.wheel.y)),
                        event.wheel.windowID});

                    break;
                }
                default:
                    break;
            }
        }
    }

    static void ProcessKeyRepeat()
    {
        if (m_heldKey == SDLK_UNKNOWN) return;

        uint64_t now = SDL_GetTicks();

        // 检查是否达到初始延迟
        if (now < m_keyPressTime + KEY_REPEAT_DELAY) return;

        // 检查是否达到重复间隔
        if (now < m_lastRepeatTime + KEY_REPEAT_INTERVAL) return;

        // 触发重复输入
        m_lastRepeatTime = now;
        handleKeyDown(m_heldKey);
        Dispatcher::Trigger<ui::events::UpdateRendering>();
    }

    static void DetailExposed()
    {
        // Add event watch to handle blocking modal loops (e.g., resizing on Windows)
        SDL_AddEventWatch(
            [](void*, SDL_Event* event) -> bool
            {
                // 对于大小改变，因为是在 Windows 消息循环中阻塞发生的，需要立即触发事件
                if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
                {
                    Dispatcher::Trigger<ui::events::WindowPixelSizeChanged>(ui::events::WindowPixelSizeChanged{
                        event->window.windowID, event->window.data1, event->window.data2});
                }
                else if (event->type == SDL_EVENT_WINDOW_MOVED)
                {
                    Dispatcher::Trigger<ui::events::WindowMoved>(
                        ui::events::WindowMoved{event->window.windowID, event->window.data1, event->window.data2});
                }

                // 窗口事件发生时立即同步窗口属性
                if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED || event->type == SDL_EVENT_WINDOW_MOVED ||
                    event->type == SDL_EVENT_WINDOW_EXPOSED || event->type == SDL_EVENT_WINDOW_SHOWN ||
                    event->type == SDL_EVENT_WINDOW_HIDDEN)
                {
                    // 查找对应的窗口实体并同步属性
                    SDL_Window* sdlWindow = SDL_GetWindowFromID(event->window.windowID);
                    if (sdlWindow != nullptr)
                    {
                        auto view = Registry::View<components::Window>();
                        for (auto entity : view)
                        {
                            auto& windowComp = view.get<components::Window>(entity);
                            if (windowComp.windowID == event->window.windowID)
                            {
                                StateSystem::syncSDLWindowProperties(entity, windowComp, sdlWindow);
                                break;
                            }
                        }
                    }
                }

                // 无论是大小改变还是窗口重绘，都需要立即重新渲染以避免黑屏或卡顿
                if (event->type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED || event->type == SDL_EVENT_WINDOW_EXPOSED)
                {
                    // 强制触发布局和渲染更新
                    Dispatcher::Trigger<ui::events::UpdateLayout>(ui::events::UpdateLayout{});
                    Dispatcher::Trigger<ui::events::UpdateRendering>(ui::events::UpdateRendering{});
                }
                return true;
            },
            nullptr);
    }

private:
    static void handleTextInput(const char* text)
    {
        auto view = Registry::View<components::FocusedTag, components::TextEdit, components::Text>();
        for (auto entity : view)
        {
            if (!Registry::AnyOf<components::TextEditTag>(entity)) continue;

            auto& edit = view.get<components::TextEdit>(entity);
            if (policies::HasFlag(edit.inputMode, policies::TextFlag::ReadOnly)) continue;

            std::string input = text != nullptr ? std::string(text) : std::string();
            if (input.empty()) continue;

            // 单行输入框过滤换行
            const auto modeVal = static_cast<uint8_t>(edit.inputMode);
            const auto multiFlag = static_cast<uint8_t>(policies::TextFlag::Multiline);
            if ((modeVal & multiFlag) == 0)
            {
                input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());
                input.erase(std::remove(input.begin(), input.end(), '\r'), input.end());
            }

            // 限制最大长度（按字节计）
            if (edit.buffer.size() + input.size() > edit.maxLength)
            {
                size_t available = edit.maxLength - edit.buffer.size();
                if (available == 0) continue;
                input = input.substr(0, available);
            }

            edit.buffer += input;

            // 同步渲染文本
            auto& textComp = view.get<components::Text>(entity);
            textComp.content = edit.buffer;

            // 标记为 Dirty
            Registry::EmplaceOrReplace<ui::components::LayoutDirtyTag>(entity);
            ui::utils::MarkRenderDirty(entity);
        }
    }

    static void handleKeyDown(SDL_Keycode key)
    {
        auto view = Registry::View<components::FocusedTag, components::TextEdit, components::Text>();
        for (auto entity : view)
        {
            if (!Registry::AnyOf<components::TextEditTag>(entity)) continue;

            auto& edit = view.get<components::TextEdit>(entity);
            if (policies::HasFlag(edit.inputMode, policies::TextFlag::ReadOnly)) continue;

            auto& textComp = view.get<components::Text>(entity);

            if (key == SDLK_BACKSPACE && !edit.buffer.empty())
            {
                edit.buffer.pop_back();
                textComp.content = edit.buffer;
                Registry::EmplaceOrReplace<ui::components::LayoutDirtyTag>(entity);
                ui::utils::MarkRenderDirty(entity);
            }
            else if (key == SDLK_RETURN)
            {
                const auto modeVal = static_cast<uint8_t>(edit.inputMode);
                const auto multiFlag = static_cast<uint8_t>(policies::TextFlag::Multiline);
                if ((modeVal & multiFlag) != 0)
                {
                    if (edit.buffer.size() + 1 <= edit.maxLength)
                    {
                        edit.buffer.push_back('\n');
                        textComp.content = edit.buffer;
                        Registry::EmplaceOrReplace<ui::components::LayoutDirtyTag>(entity);
                        ui::utils::MarkRenderDirty(entity);
                    }
                }
            }
            // 注意：普通字符输入由 SDL_EVENT_TEXT_INPUT 处理
        }
    }

    // 键盘长按状态
    inline static SDL_Keycode m_heldKey = SDLK_UNKNOWN;        // 当前按下的按键
    inline static uint64_t m_keyPressTime = 0;                 // 按键按下时间（毫秒）
    inline static uint64_t m_lastRepeatTime = 0;               // 上次重复输入时间
    inline static constexpr uint64_t KEY_REPEAT_DELAY = 500;   // 长按触发延迟（毫秒）
    inline static constexpr uint64_t KEY_REPEAT_INTERVAL = 50; // 重复输入间隔（毫秒）
};

} // namespace ui::systems