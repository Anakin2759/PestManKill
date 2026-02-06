# mainwindow show 小概率白屏问题记录

## 现象

- 单线程 UI 程序中，调用 `CreateMainWindow()` 后 `ui::visibility::Show()`，偶发白屏。
- 白屏后窗口没有后续渲染提交，除非有用户输入等触发新的渲染脏标记。

## 直接原因

`RenderSystem::update()` 仅渲染带 `RenderDirtyTag` 的窗口，并在函数末尾无条件清除所有 `RenderDirtyTag`。当窗口刚被 `Show` 时，`SDL_GetWindowSizeInPixels` 有小概率返回 $0\times0$，该帧跳过渲染提交，但 `RenderDirtyTag` 仍被清空，后续 `UpdateRendering` 触发也无法进入渲染流程，从而形成白屏。

## 证据与链路

- `Show()` 在窗口显示后只标记 Layout/Render Dirty，并不强制提交渲染帧。[src/ui/api/Visibility.cpp](src/ui/api/Visibility.cpp#L18-L63)
- `RenderSystem::update()` 只遍历含 `RenderDirtyTag` 的窗口，并在窗口像素尺寸为 0 时直接跳过渲染；函数末尾会无条件移除所有 `RenderDirtyTag`。[src/ui/systems/RenderSystem.cpp](src/ui/systems/RenderSystem.cpp#L214-L302)
- `UpdateRendering` 由定时任务或窗口暴露事件触发，但不会重新设置 `RenderDirtyTag`。[src/ui/core/TaskChain.hpp](src/ui/core/TaskChain.hpp#L103-L120) 与 [src/ui/systems/InteractionSystem.hpp](src/ui/systems/InteractionSystem.hpp#L245-L280)
- `MarkRenderDirty()` 只在显式调用时才打标记。[src/ui/api/Utils.cpp](src/ui/api/Utils.cpp#L16-L46)

## 复现条件（推测）

1. `CreateMainWindow()` 创建窗口后紧接 `Show()`。
2. `Show()` 触发首帧渲染脏标记。
3. 首次 `RenderSystem::update()` 中，`SDL_GetWindowSizeInPixels` 返回 $0\times0$（窗口未完全可见/未完成像素尺寸同步）。
4. 渲染被跳过，但 `RenderDirtyTag` 被清空，后续 `UpdateRendering` 无法进入渲染流程，导致白屏持续。

## 结论

白屏是一次性渲染脏标记被清空导致的“渲染机会丢失”。根因是渲染系统对 $0\times0$ 尺寸的窗口提前退出并清理 `RenderDirtyTag`，而后续渲染触发不补充脏标记。
