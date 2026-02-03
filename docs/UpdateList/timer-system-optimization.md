# 定时器系统优化方案 (Timer System Optimization)

## 1. 现状分析 (Current Status Analysis)

当前 `ui::utils::TimerCallback` 基于 ECS 的事件分发器 (`Dispatcher`) 实现，存在以下问题：

1. **性能开销大 (High Overhead)**:
    * **反复入队 (Ping-pong Enqueue)**: `QueuedTask` 被设计为一个事件。为了维持定时器状态，`ActionSystem` 在每帧处理完事件后，如果定时器未触发，会将该事件重新 `Enqueue` 回 `Dispatcher`。
    * 这导致每一帧、每一个存活的定时器都会发生一次 `std::vector` 的 `push_back` (主要是在 `Dispatcher` 内部 sink 的 buffer 中) 和潜在的内存分配/拷贝（尽管是 move，但容器操作仍有开销）。
    * 对于高频刷新或大量定时器的场景，这种"每帧搬运"的模式极其低效。

2. **功能缺失 (Missing Functionality)**:
    * `TimerCallback` 目前返回固定值 `0`，无法区分任务。
    * `CancelQueuedTask` 仅有声明，没有实现。由于事件一旦入队就“淹没”在 Dispatcher 的缓冲中，且每帧都在变动位置，难以通过 Handle 定位并取消特定任务。

3. **架构滥用 (Architecture Abuse)**:
    * 事件系统应是一次性的（Fire-and-forget）。将事件系统用作状态存储（虽然是暂时的）违反了设计原则。

## 2. 优化目标 (Optimization Goals)

1. **独立管理**: 引入 `TimerManager` 单例，独立管理定时任务生命周期。
2. **高效调度**: 使用合适的数据结构（如优先队列 `std::priority_queue` 或时间轮），避免对所有定时器的全量遍历，消除无谓的每帧内存操作。
3. **完整功能**: 实现可靠的 `TaskHandle` 分配机制，支持 `Cancel` 操作。
4. **接口兼容**: 保持 `ui::utils::TimerCallback` 接口不变（或微调语义），底层替换实现。

## 3. 重构方案 (Refactoring Proposal)

### 3.1 核心数据结构

不再使用 `entt::dispatcher` 存储待执行任务。

```cpp
// 任务句柄定义
using TaskHandle = uint32_t;

struct TimerTask {
    TaskHandle handle;
    std::move_only_function<void()> callback;
    uint32_t intervalMs;
    uint32_t nextRunTimeMs; // 绝对时间戳或累积时间
    bool isRepeating;
    bool markedForDeletion; // 惰性删除标记
    
    // 用于优先队列排序，执行时间早的排在前面
    bool operator>(const TimerTask& other) const {
        return nextRunTimeMs > other.nextRunTimeMs;
    }
};
```

### 3.2 TimerManager 模块

引入 `Singleton` : `ui::managers::TimerManager`。

* **容器**:
  * 鉴于 UI 场景定时器数量级较小，且需要极其方便的 Cancel 操作，建议采用 **ID索引的惰性删除堆方案**。
  * `std::vector<TimerTask>` + `std::push_heap/pop_heap` 维护最小堆。
  * `std::unordered_set<TaskHandle> m_cancelledHandles` 记录被取消的任务 ID。

* **逻辑**:
  * `Update(uint32_t dt)`: 增加全局时间累积器 `m_currentTime`。
  * 循环检查堆顶任务：
    * 若 `m_cancelledHandles` 包含该任务 ID -> `pop_heap` 移除，不做任何事。
    * 若 `task.nextRunTimeMs <= m_currentTime` -> `pop_heap` 移除并执行。
    * 执行后，若是循环任务 -> 计算新时间 `nextRunTimeMs += intervalMs` -> `push_heap` 重新入堆。
  * 若堆顶时间未到 -> 结束本帧检查。

### 3.3 代码变更计划

1. **创建 `src/ui/managers/TimerManager.hpp/cpp`**:

    ```cpp
    class TimerManager {
    public:
        static TimerManager& Instance();
        
        TaskHandle AddTimer(uint32_t intervalMs, bool repeating, std::move_only_function<void()> func);
        void CancelTimer(TaskHandle handle);
        void Update(uint32_t dt); // 每帧调用
        
    private:
        struct TimerTask { ... };
        std::vector<TimerTask> m_heap; 
        std::unordered_set<TaskHandle> m_cancelled;
        uint32_t m_globalTime = 0;
        uint32_t m_nextHandleId = 1;
    };
    ```

2. **修改 `src/ui/api/Utils.cpp`**:
    * `TimerCallback`: 转发调用到 `TimerManager::Instance().AddTimer(...)`。
    * `CancelQueuedTask`: 转发调用到 `TimerManager::Instance().CancelTimer(...)`。

3. **修改 `src/ui/core/TaskChain.hpp`**:
    * 在主循环中添加 `TimerManager::Instance().Update(dt)`。

4. **移除旧代码**:
    * 删除 `ui::systems::ActionSystem::onQueuedTask` 中的定时器逻辑。
    * 清理 `Events.hpp` 中 `QueuedTask` 不再需要的字段（`intervalMs`, `remainingMs`, `singleShoot` 等）。只保留 `func` 用于某些一次性派发场景（如果还有保留必要），或者彻底废弃该事件。
    * *注*: `utils::InvokeTask` 是一次性立即（下一帧）执行，也可以纳入 `TimerManager` (interval=0) 管理，或者保留走 Dispatcher 流程。建议统一纳入 TimerManager (delay=0)，除非有特定的 System 顺序要求。

## 4. 详细设计 (Implementation Details)

### 4.1 Handle 生成

使用自增 `uint32_t`。为防止溢出回绕导致的 ID 冲突，可配合 `generation` 字段，或者假设 40 亿次分配在单次运行周期内不会耗尽。

### 4.2 时间管理

使用单调递增的“虚拟时间戳” (`accumulatedTime`).

* Current Time: `0`
* Task A (delay 100ms): `Target = 100`
* Update(dt=16): `Current = 16`. Check `16 >= 100` (False).

### 4.3 线程安全

当前 UI 系统运行在主线程模型。`TimerManager` 内暂不需要锁，除非后续有跨线程投递定时任务的需求（届时加 `std::mutex` 保护 `AddTimer` 和 `Update`）。

## 5. 预期收益 (Expected Benefits)

1. **CPU 占用降低**: 消除空转定时器的每帧内存分配和移动开销。
2. **API 可用性**: `Cancel` 接口生效，支持更复杂的 UI 交互逻辑（如长按取消、防抖等）。
3. **代码清晰度**: 逻辑解耦，System 关注业务，Manager 关注调度。
