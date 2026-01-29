# 状态更新合并优化

## 概述

为了减少 `StateSystem` 中频繁的 Registry 操作开销，实现了同帧内状态更新合并机制，通过延迟执行和批量处理来优化性能。

## 问题分析

### 优化前的问题

1. **高频 Registry 操作**：每次状态变化都立即调用 `Registry::Remove` 和 `Registry::EmplaceOrReplace`
2. **重复操作**：同一帧内可能对同一实体进行多次添加/移除操作
3. **性能浪费**：在鼠标快速移动场景下，Hover 状态可能频繁切换
4. **无效更新**：可能出现"添加后立即移除"或"移除后立即添加"的情况

### 典型场景

```
帧 N:
  T1: 鼠标移入实体 A  → 添加 HoveredTag(A)
  T2: 鼠标移出实体 A  → 移除 HoveredTag(A)  ← 无效操作
  T3: 鼠标移入实体 B  → 添加 HoveredTag(B)
  
结果：执行了 3 次 Registry 操作，但实际只需要 1 次（添加 B）
```

## 优化方案

### 1. 延迟应用机制

将状态更新标记存入待处理队列，在帧结束时统一应用：

```cpp
// 待处理的状态更新集合（自动去重）
std::unordered_set<entt::entity> m_pendingHoverAdd;
std::unordered_set<entt::entity> m_pendingHoverRemove;
std::unordered_set<entt::entity> m_pendingActiveAdd;
std::unordered_set<entt::entity> m_pendingActiveRemove;
```

### 2. 合并冲突操作

在添加操作时，自动取消对同一实体的移除操作，反之亦然：

```cpp
// 示例：添加 Hover 时取消移除
m_pendingHoverAdd.insert(event.entity);
m_pendingHoverRemove.erase(event.entity);  // 合并冲突
```

### 3. 帧结束批量应用

在 `EndFrame` 事件中统一应用所有状态更新：

```cpp
void onEndFrame()
{
    // 1. 移除状态
    for (entt::entity entity : m_pendingHoverRemove)
        Registry::Remove<components::HoveredTag>(entity);
    
    // 2. 添加状态
    for (entt::entity entity : m_pendingHoverAdd)
        Registry::EmplaceOrReplace<components::HoveredTag>(entity);
    
    // 3. 清空队列
    m_pendingHoverAdd.clear();
    m_pendingHoverRemove.clear();
}
```

## 实现细节

### 受影响的状态

| 状态标签 | 触发场景 | 合并策略 |
|---------|---------|---------|
| `HoveredTag` | 鼠标移入/移出 | 延迟应用 |
| `ActiveTag` | 鼠标按下/释放 | 延迟应用 |
| `FocusedTag` | 输入框焦点 | **立即应用** |

### 特殊处理：Focus 状态

Focus 状态**不使用**延迟应用，因为需要同步 SDL 输入法状态：

```cpp
// Focus 立即应用（涉及 SDL API 调用）
static void setFocus(entt::entity entity, SDL_Window* sdlWindow)
{
    // 立即移除旧焦点
    Registry::Remove<components::FocusedTag>(state.focusedEntity);
    
    // 立即添加新焦点
    Registry::EmplaceOrReplace<components::FocusedTag>(entity);
    
    // 同步 SDL 输入法状态
    SDL_StartTextInput(sdlWindow);
}
```

### 事件流程

```
帧开始
  ↓
输入处理 → 标记状态更新（入队列）
  ↓
布局计算
  ↓
渲染绘制
  ↓
【帧结束】→ 批量应用状态更新（出队列）
  ↓
下一帧
```

## 性能提升

### 理论收益

| 场景 | 优化前操作数 | 优化后操作数 | 减少比例 |
|-----|-----------|------------|---------|
| 鼠标快速移动 | ~30 次/帧 | ~2 次/帧 | 93% ↓ |
| 连续点击 | ~6 次/事件 | ~2 次/事件 | 66% ↓ |
| 静态悬停 | ~2 次/帧 | ~1 次/帧 | 50% ↓ |

### 实际效果

- **减少 Registry 信号触发**：避免大量的 `on_construct` / `on_destroy` 回调
- **降低缓存失效频率**：减少对其他系统的连锁影响（如 HitTestSystem 缓存）
- **提高帧率稳定性**：消除因状态抖动导致的性能波动

## 优化对比

### 优化前代码

```cpp
void onHoverEvent(const events::HoverEvent& event)
{
    // 立即移除旧状态
    if (state.hoveredEntity != entt::null)
        Registry::Remove<components::HoveredTag>(state.hoveredEntity);
    
    // 立即添加新状态
    Registry::EmplaceOrReplace<components::HoveredTag>(event.entity);
}
```

### 优化后代码

```cpp
void onHoverEvent(const events::HoverEvent& event)
{
    // 标记待移除
    if (state.hoveredEntity != entt::null)
        m_pendingHoverRemove.insert(state.hoveredEntity);
    
    // 标记待添加（并取消可能的移除）
    m_pendingHoverAdd.insert(event.entity);
    m_pendingHoverRemove.erase(event.entity);  // 合并冲突
}

// 帧结束时统一应用
void onEndFrame()
{
    for (auto e : m_pendingHoverRemove)
        Registry::Remove<components::HoveredTag>(e);
    for (auto e : m_pendingHoverAdd)
        Registry::EmplaceOrReplace<components::HoveredTag>(e);
    
    m_pendingHoverAdd.clear();
    m_pendingHoverRemove.clear();
}
```

## 兼容性

### API 稳定性

- ✅ 外部接口无变化
- ✅ 事件处理流程不变
- ✅ 对其他系统透明

### 行为差异

- ⚠️ 状态更新延迟到帧结束
- ⚠️ 同帧内查询状态可能不准确（实际场景极少）
- ✅ 最终状态一致性保证

## 后续优化方向

1. **动态调整合并窗口**
   - 根据帧率自适应调整批量应用的时机
   - 高帧率下可以更激进地合并

2. **优先级队列**
   - 重要状态优先应用
   - 降低关键交互的延迟感知

3. **脏标记传播优化**
   - 状态更新时智能标记受影响的渲染区域
   - 避免全屏重绘

4. **统计与监控**
   - 记录合并效率（避免的操作数）
   - 性能指标可视化

## 代码变更

### 修改文件

1. **StateSystem.hpp**
   - 添加待处理队列成员变量
   - 修改 `onHoverEvent` / `onUnhoverEvent` / `onMousePressEvent` / `onMouseReleaseEvent`
   - 新增 `onEndFrame` 方法
   - 注册 `EndFrame` 事件监听

2. **Events.hpp**
   - 新增 `EndFrame` 事件定义

3. **TaskChain.hpp**
   - 在 `RenderTask` 渲染循环中触发 `EndFrame` 事件

### 新增依赖

```cpp
#include <unordered_set>  // StateSystem.hpp
```

---

**版本**：v0.3  
**日期**：2026-01-29  
**作者**：AnakinLiu
