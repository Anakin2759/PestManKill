## UI 模块架构说明

### 设计原则

采用**依赖注入**而非单例模式，提高代码的可测试性和灵活性。

### 核心组件

#### 1. GraphicsContext（图形上下文）

- **非单例设计**：作为普通类，由 Application 持有
- **职责**：封装 SDL 窗口和渲染器的访问接口
- **生命周期**：由 Application 管理
- **优点**：
  - 便于单元测试（可以 mock）
  - 支持未来的多窗口场景
  - 减少全局状态，降低耦合

```cpp
class GraphicsContext {
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    // ...
};
```

#### 2. Application（应用程序）

- **职责**：
  - 创建和管理 SDL 窗口/渲染器
  - 持有 GraphicsContext 实例
  - 通过依赖注入将 GraphicsContext 传递给 SystemManager
  - 管理主事件循环和时间更新
  
```cpp
class Application {
private:
    GraphicsContext m_graphicsContext;  // 持有实例
    SystemManager m_systems;            // 系统管理器
    
public:
    Application() {
        // 创建 SDL 资源
        // 初始化 GraphicsContext
        m_graphicsContext.initialize(window, renderer, width, height);
        
        // 注入到 SystemManager
        m_systems.setGraphicsContext(&m_graphicsContext);
    }
};
```

#### 3. SystemManager（系统管理器）

- **职责**：
  - 管理所有 UI 系统的生命周期
  - 按正确顺序调用系统更新
  - 将 GraphicsContext 注入到需要的系统

```cpp
class SystemManager {
private:
    GraphicsContext* m_graphicsContext;  // 引用，不拥有
    
public:
    void setGraphicsContext(GraphicsContext* ctx) {
        m_graphicsContext = ctx;
        m_renderSystem.setGraphicsContext(ctx);  // 传递给需要的系统
    }
};
```

#### 4. RenderSystem（渲染系统）

- **职责**：渲染所有 UI 元素
- **依赖**：通过依赖注入接收 GraphicsContext 引用

```cpp
class RenderSystem {
private:
    GraphicsContext* m_graphicsContext;  // 引用，不拥有
    
public:
    void setGraphicsContext(GraphicsContext* ctx) {
        m_graphicsContext = ctx;
    }
    
    void updateImpl() {
        SDL_Renderer* renderer = m_graphicsContext->getRenderer();
        // 渲染逻辑...
    }
};
```

### 依赖关系

```
Application
  ├─ GraphicsContext (拥有)
  └─ SystemManager (拥有)
       ├─ InteractionSystem
       ├─ AnimationSystem
       ├─ LayoutSystem
       ├─ RenderSystem (引用 GraphicsContext)
       └─ WindowSystem
```

### 为什么不用单例？

1. **可测试性**：单例难以 mock，依赖注入便于单元测试
2. **灵活性**：未来如果需要多窗口，单例会成为限制
3. **生命周期清晰**：资源的创建和销毁由 Application 明确管理
4. **减少全局状态**：全局可变状态是 bug 的常见来源
5. **依赖关系显式**：通过参数传递，依赖关系一目了然

### 使用示例

```cpp
// 创建自定义应用
class MyApp : public ui::Application {
protected:
    void setupUI() override {
        // 创建 UI 元素
        auto button = createButton(...);
        // ...
    }
};

// 运行应用
int main() {
    try {
        MyApp app;
        app.run();
    } catch (const std::exception& e) {
        // 错误处理
    }
    return 0;
}
```

### 总结

- ✅ Application 持有并管理 GraphicsContext
- ✅ 通过依赖注入传递给需要的系统
- ✅ 避免全局单例，提高可测试性
- ✅ 生命周期清晰，资源管理安全
- ✅ 架构灵活，便于未来扩展
