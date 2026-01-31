# UI 模块下一步更新计划

## 1. Icon 系统全面落地 (Priority: High)

目前 Icon 系统已有初步架构，但尚未完全跑通。

- [ ] **IconManager 补完**: 实现从 TTF 码点生成位图并缓存到 GPU 纹理的逻辑。
- [ ] **IconRenderer 集成**: 恢复 `RenderSystem` 中被注释掉的 `IconRenderer`，实现基于矩形和白色纹理（或 Icon 图集）的图标渲染。
- [ ] **API 提供**: 在 `Factory.hpp` 中暴露 `SetIcon` 方法，支持为按钮和标签快速挂载图标。
- [ ] **资源整合**: 扫描 `ui/assets/icons` 下的图标库并自动加载。

## 2. 交互体验增强 (Priority: Medium)

- [ ] **滚动条完善**: 补全 `ScrollBarRenderer` 与 `StateSystem` 中的水平滚动条逻辑。
- [ ] **状态反馈**: 为滚动条滑块 (Thumb) 添加 Hover 和 Active 视觉状态反馈。
- [ ] **Tooltips 基础**: 基于 `ZOrder` 实现浮动提示层（Tooltip）的逻辑框架。

## 3. 渲染效率优化 (Priority: Medium)

- [ ] **Batching 深度优化**: 在 `BatchManager::optimize()` 中实现按纹理和裁剪区域 (Scissor) 进行的批次排序与合并，显著降低 Draw Call。
- [ ] **缓冲区池化**: 在 `CommandBuffer` 中引入缓冲区池，避免每一帧重复创建和销毁 GPU 资源。

## 4. 动画系统雏形 (Priority: Low)

- [ ] **补间组件**: 引入 `Tween` 组件，支持对 `Alpha`、`Position` 等组件进行时间轴插值。
- [ ] **交互动效**: 实现按钮点击时的缩放或颜色渐变效果。

## 5. 主题系统 (Priority: Low)

- [ ] **样式抽离**: 将常用的颜色、圆角、间距等硬编码参数整合为 `Theme` 资源。
- [ ] **动态切换**: 支持在运行时一键切换全局主题方案（如 Dark/Light Mode）。
