# RenderSystem 重构方案

## 当前问题分析

### 1. 代码规模与职责过载

- **RenderSystem.hpp**: 1725行代码，单一类承担过多职责
- 混合了多种职责：数据收集、几何生成、文本处理、批次管理、GPU渲染

### 2. 缺乏分层架构

- 所有渲染逻辑集中在一个类中
- 难以扩展新的渲染类型
- 难以单独测试各个功能模块

### 3. 代码重复与可维护性

- 类似的渲染逻辑（矩形、圆角、阴影）重复出现
- 批次管理逻辑与具体渲染混合
- 资源管理（缓冲区上传）每次都重复创建

## 重构目标

### 1. 分层架构

- **渲染器层(Renderers)**: 专门处理特定类型的渲染
- **批处理层(Batcher)**: 负责批次组装和优化
- **命令层(Commands)**: 封装GPU命令
- **协调层(RenderSystem)**: 精简为调度器角色

### 2. 单一职责

- 每个渲染器只负责一种类型的渲染
- BatchManager专注批次优化
- CommandBuffer专注GPU命令封装

### 3. 可扩展性

- 新增渲染类型只需添加新的Renderer
- 渲染器之间互不影响
- 易于单独测试

## 新架构设计

```
RenderSystem (调度器)
├── RenderPipeline (渲染管线)
│   ├── CollectPhase (收集阶段)
│   │   └── 遍历ECS收集渲染数据
│   ├── BatchPhase (批次阶段)
│   │   └── BatchManager组装批次
│   └── ExecutePhase (执行阶段)
│       └── CommandBuffer执行GPU命令
│
├── Renderers (渲染器层)
│   ├── IRenderer (接口)
│   ├── ShapeRenderer (形状渲染器)
│   │   ├── 矩形
│   │   ├── 圆角矩形
│   │   └── 阴影
│   ├── TextRenderer (文本渲染器)
│   │   ├── 单行文本
│   │   ├── 多行文本
│   │   └── 文本换行
│   ├── IconRenderer (图标渲染器)
│   └── ScrollBarRenderer (滚动条渲染器)
│
├── BatchManager (批次管理器)
│   ├── 批次合并
│   ├── 状态排序
│   └── 纹理打包
│
├── CommandBuffer (命令缓冲区)
│   ├── GPU命令封装
│   ├── 资源上传
│   └── 缓冲区池化
│
└── Managers (资源管理器)
    ├── DeviceManager
    ├── FontManager
    ├── PipelineCache
    └── TextTextureCache
```

## 实现计划

### Phase 1: 基础架构

1. 创建 `IRenderer` 接口
2. 创建 `RenderContext` 上下文对象
3. 创建 `BatchManager` 类
4. 创建 `CommandBuffer` 包装器

### Phase 2: 渲染器实现

1. `ShapeRenderer`: 处理所有形状渲染
2. `TextRenderer`: 处理所有文本渲染
3. `IconRenderer`: 处理图标渲染
4. `ScrollBarRenderer`: 处理滚动条渲染

### Phase 3: 重构RenderSystem

1. 精简为调度器
2. 集成各个渲染器
3. 优化渲染管线

### Phase 4: 优化与测试

1. 缓冲区池化
2. 批次优化
3. 单元测试
4. 性能测试

## 代码示例

### IRenderer接口

```cpp
class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void collect(entt::entity entity, RenderContext& ctx) = 0;
};
```

### RenderContext

```cpp
struct RenderContext {
    Eigen::Vector2f position;
    Eigen::Vector2f size;
    float alpha;
    BatchManager& batchManager;
    // ... 其他共享状态
};
```

### 精简后的RenderSystem

```cpp
class RenderSystem {
    void update() {
        // 1. 收集阶段
        for (auto entity : windowView) {
            collectRenderData(entity);
        }
        
        // 2. 批次优化
        m_batchManager.optimize();
        
        // 3. GPU执行
        m_commandBuffer.execute(m_batchManager.getBatches());
    }
};
```

## 优势

1. **模块化**: 每个组件职责清晰，易于理解和维护
2. **可测试性**: 每个渲染器可以独立测试
3. **可扩展性**: 新增渲染类型无需修改现有代码
4. **性能**: 批次管理和资源池化提升性能
5. **代码重用**: 通用逻辑抽取到基类或工具函数

## 向后兼容

重构过程中保持对外接口不变，确保其他系统不受影响。
