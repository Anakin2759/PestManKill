#pragma once

#include "../common/Events.hpp"

#include "Contains.hpp"
#ifdef CreateWindow
#undef CreateWindow
#endif
namespace ui::traits
{
// ===================== 组件列表 =====================
using events = TypeList<events::ApplicationReadyEvent,
                        events::QuitRequested,
                        events::WindowMoved,
                        events::WindowPixelSizeChanged,
                        events::WindowGraphicsContextSetEvent,
                        events::WindowGraphicsContextUnsetEvent,
                        events::ValueChangedSelection,
                        events::SendHandlerToEventLoop,
                        events::UpdateEvent,
                        events::CreateWindow,
                        events::CloseWindow,
                        events::UpdateRendering,
                        events::UpdateLayout,
                        events::QueuedTask,
                        events::SDLEvent,
                        events::ClickEvent,
                        events::HoverEvent,
                        events::UnhoverEvent,
                        events::MousePressEvent,
                        events::MouseReleaseEvent>;
// ===================== 策略检测 =====================

/**
 * @brief
 */
template <typename T>
inline constexpr bool is_events_v = contains_v<T, events>; // NOLINT

template <typename T>
concept Events = is_events_v<T>;

} // namespace ui::traits