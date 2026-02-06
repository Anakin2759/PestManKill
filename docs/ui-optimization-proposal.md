# UI 模块优化建议文档

**文档名称**: UI Module Optimization Proposal  
**创建日期**: 2026年2月6日  
**状态**: 草稿 (Draft)

## 1. 概述

本文档旨在记录和规划 UI 模块的性能优化方向。主要关注点包括渲染批次处理 (Batching)、内存管理、以及多线程渲染的可能性。

## 2. 现有架构分析 (Based on BatchManager)

当前 `BatchManager` 使用了 `std::pmr` 进行内存管理，并实现了基础的批次合并逻辑（基于 Texture, Scissor Rect, Push Constants）。

### 2.1 优点

- **内存池**: 使用 `std::pmr::monotonic_buffer_resource` 有效减少了每帧的小对象分配开销。
- **自动合并**: `beginBatch` 中包含自动检测并合并相邻批次的逻辑。

### 2.2 潜在瓶颈

- **顺序依赖**:目前的合并逻辑仅依赖于提交顺序。如果两个相同状态的 draw call 中间夹杂了一个不同状态的 draw call，将会导致批次被打断。
- **比较开销**: `pushConstants` 的比较使用了逐字段浮点比较，虽然精确但在高频调用下可能有开销。

## 3. 优化建议

### 3.1 渲染层优化 (Rendering)

#### 3.1.1 引入 Render Key 排序 (Render Key Sorting)

**现状**: 按提交顺序绘制 (Painter's Algorithm)。
**建议**: 引入 `RenderKey` 概念。

- 将 深度(Layer/Z-index) + 纹理ID + ShaderID + ScissorID 编码为一个 64位 整数。
- 在每帧渲染前，先收集所有 DrawCommand，根据 RenderKey 进行排序。
- **收益**: 极大减少 State Change（纹理切换、管线切换），提高批次合并率。
- **注意**: 对于半透明物体，需要严格保持从后往前的渲染顺序，可能限制排序的灵活性。

#### 3.1.2 优化参数比较

**现状**: `beginBatch` 中对 `PushConstants` 进行成员逐一比较。
**建议**:

- 确保 `UiPushConstants` 结构体内存布局紧凑且已初始化（无未定义 padding）。
- 使用 `std::memcmp` 或重载 `operator==` 进行快速比较。
- 或者引入 "Material 句柄" 概念，仅比较指针或 ID。

### 3.2 内存管理优化 (Memory)

#### 3.2.1 动态扩容策略

**现状**: `m_bufferResource(256 * 1024)` 硬编码了初始大小。
**建议**:

- 增加统计机制，记录最近 N 帧的平均内存使用量。
- 动态调整下一帧的预分配大小，避免过大浪费或过小导致的 `upstream` 分配。

#### 3.2.2 顶点数据结构

**建议**: 检查 `Vertex` 结构体是否对其到 cache line 或 GPU 友好的对齐方式 (如 16 字节对齐)。

### 3.3 数据结构与算法

#### 3.3.1 批次合并逻辑分离

**现状**: 合并逻辑耦合在 `beginBatch` 中。
**建议**:

- 采用 "Submit -> Sort -> Bake" 的流程。
- `beginBatch` 仅负责记录命令。
- `endFrame` 时统一处理排序和合并。

### 3.4 调试与工具

- **Draw Call 可视化**: 开发一个 overlay 显示当前的 Draw Call 数量、批次数量和顶点数量。
- **GPU 调试集成**: 集成 RenderDoc 或 Nsight 标记，便于分析 GPU 耗时。

## 4. 实施路线图

1. **P0**: 实现 Draw Call 统计与可视化 (基准数据)。
2. **P1**: 优化 `beginBatch` 中的比较逻辑。
3. **P2**: 原型验证 "Render Key 排序" 对非透明 UI 的效果。
4. **P3**: 完善内存池的统计与自适应调整。

## 5. 参考资料

- [SDL3 GPU 最佳实践]
- [Modern OpenGL/Vulkan Batching Strategies]
