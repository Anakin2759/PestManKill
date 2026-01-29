# 命中测试缓存优化

## 概述

为了减少输入事件处理中的性能开销，实现了基于 `ZOrderIndex` 组件的命中测试缓存机制。

## 优化内容

### 1. 缓存机制

- **按窗口维护 Z-Order 缓存**：每个窗口维护一个独立的排序实体列表
- **懒加载**：只在缓存失效时重新计算，避免每次输入事件都进行全量排序
- **结构**：

  ```cpp
  struct ZOrderCache {
      std::vector<entt::entity> entities; // 排序后的实体列表
      bool dirty = true;                  // 缓存失效标记
  };
  ```

### 2. Z-Order 排序优化

- **优先使用 `ZOrderIndex` 组件**：显式控制元素的前后显示顺序
- **回退到深度排序**：对于没有 `ZOrderIndex` 的实体，使用层级深度作为排序依据
- **排序规则**：Z-Order 值越大，越靠近前端（优先响应输入）

### 3. 缓存失效机制

实现了细粒度的缓存失效策略，监听以下组件变化：

| 组件变化 | 失效范围 | 原因 |
|---------|---------|------|
| `ZOrderIndex` | 所属窗口 | 元素层级顺序改变 |
| `Hierarchy` | 全局 | 父子关系变化可能影响多个窗口 |
| `VisibleTag` | 所属窗口 | 可见性改变影响可交互实体列表 |

### 4. 实现细节

#### 信号连接

```cpp
// 注册缓存失效的信号监听
Registry::GetRegistry().on_construct<components::ZOrderIndex>().connect<&HitTestSystem::onZOrderChanged>(*this);
Registry::GetRegistry().on_update<components::ZOrderIndex>().connect<&HitTestSystem::onZOrderChanged>(*this);
Registry::GetRegistry().on_destroy<components::ZOrderIndex>().connect<&HitTestSystem::onZOrderChanged>(*this);
```

#### 排序逻辑

```cpp
// 优先使用 ZOrderIndex，否则使用深度
const auto* zOrderComp = Registry::TryGet<components::ZOrderIndex>(entity);
if (zOrderComp) {
    zOrder = zOrderComp->value;
} else {
    // 回退到深度排序
    zOrder = calculateDepth(entity);
}
```

## 性能提升

### 优化前

- **每次输入事件**都需要：
  1. 遍历所有可交互实体
  2. 计算深度或 Z-Order
  3. 执行排序算法 O(n log n)

### 优化后

- **命中时**：
  1. 检查缓存是否有效 O(1)
  2. 直接使用缓存结果 O(1)
  3. 仅在缓存失效时重建 O(n log n)

### 预期收益

- **高频输入场景**：鼠标移动、连续点击等，性能提升最显著
- **复杂 UI**：可交互元素越多，优化效果越明显
- **理论减少**：每帧节省 (n × log n) 次比较操作

## 使用建议

### 1. 为需要精确控制层级的元素添加 `ZOrderIndex`

```cpp
// 创建按钮并设置 Z-Order
auto button = registry.create();
registry.emplace<components::ZOrderIndex>(button, 100); // 高优先级
```

### 2. 避免频繁修改层级结构

- 尽量在初始化时设置好 Z-Order
- 批量修改后一次性更新

### 3. 合理使用窗口隔离

- 每个窗口维护独立缓存
- 避免跨窗口的复杂交互

## 兼容性

- **向后兼容**：未使用 `ZOrderIndex` 的代码自动回退到深度排序
- **无破坏性变更**：现有 API 保持不变
- **性能透明**：优化对使用者完全透明

## 后续优化方向

1. **空间分区**：对于大量元素的场景，可考虑四叉树等空间分区算法
2. **增量更新**：针对局部变化，仅更新受影响的部分
3. **多线程支持**：将排序计算移至后台线程

---

**修改文件**：

- `src/ui/systems/HitTestSystem.hpp`

**版本**：v0.3  
**日期**：2026-01-29  
**作者**：AnakinLiu
