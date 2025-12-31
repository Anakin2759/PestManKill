## UI 模块架构说明 v2

### 设计原则

1. **组件化设计**：每个功能模块独立封装
2. **依赖注入**：而非单例模式，提高可测试性
3. **职责分离**：各组件职责清晰，易于维护

### 核心组件层次

```
Application (应用程序容器)
  ├─ GraphicsContext      (图形上下文)
  ├─ ImguiContext        (ImGui 管理)
  ├─ EventLoop           (事件循环)
  └─ SystemManager       (系统管理器)
       └─ entt::poly<ISystem>[]  (ECS 系统数组)
```

---

### 1. GraphicsContext（图形上下文）

**职责**：封装 SDL 窗口和渲染器

```cpp
class GraphicsContext {
private:
    SDL_Window* m_window;
    SDL_Renderer* m_renderer;
    int m_width, m_height;
    
public:
    void initialize(SDL_Window*, SDL_Renderer*, int w, int h);
    void handleResize(int w, int h);
    
    SDL_Window* getWindow() const;
    SDL_Renderer* getRenderer() const;
};
```

**特点**：

- ✅ 非单例设计，支持多窗口
- ✅ 通过依赖注入传递
- ✅ 生命周期由 Application 管理

---

### 2. ImguiContext（ImGui 上下文）

**职责**：封装 ImGui 的初始化和清理

```cpp
class ImguiContext {
public:
    ImguiContext(SDL_Window*, SDL_Renderer*);  // 初始化 ImGui
    ~ImguiContext();                            // 清理 ImGui
    
    // 禁用拷贝和移动
};
```

**特点**：

- ✅ RAII 模式，自动管理生命周期
- ✅ 构造时初始化，析构时清理
- ✅ 避免 Application 直接调用 ImGui API

**使用示例**：

```cpp
// Application 中
m_imguiContext = std::make_unique<ImguiContext>(window, renderer);
// 析构时自动清理，无需手动调用 ImGui_Impl*_Shutdown()
```

---

### 3. EventLoop（事件循环）

**职责**：基于 ASIO 的异步事件调度

```cpp
class EventLoop {
private:
    std::unique_ptr<asio::io_context> m_ioContext;
    asio::executor_work_guard<...> m_workGuard;
    
public:
    void exec();                              // 启动事件循环
    void quit();                              // 停止事件循环
    
    template<typename F>
    void invoke(F&& func);                    // 投递任务到事件循环
    
    asio::io_context& getContext();
};
```

**特点**：

- ✅ 异步任务投递
- ✅ 跨平台事件调度
- ✅ 支持未来的异步 UI 更新

**使用示例**：

```cpp
// 投递异步任务
app.getEventLoop().invoke([&]() {
    // 在事件循环中执行
    updateUI();
});
```

---

### 4. SystemManager（系统管理器）

**职责**：使用 `entt::poly` 动态管理 ECS 系统

```cpp
class SystemManager {
private:
    GraphicsContext* m_graphicsContext;
    std::vector<entt::poly<ISystem>> m_systems;
    
    enum SystemIndex { INTERACTION, ANIMATION, LAYOUT, RENDER, WINDOW };
    
public:
    SystemManager() {
        // 按顺序添加系统
        m_systems.emplace_back(InteractionSystem{});
        m_systems.emplace_back(AnimationSystem{});
        m_systems.emplace_back(LayoutSystem{});
        m_systems.emplace_back(RenderSystem{});
        m_systems.emplace_back(WindowSystem{});
    }
    
    void setGraphicsContext(GraphicsContext*);
    void registerAllHandlers();
    void update(float deltaTime);
    void render();
    
    // 动态系统管理
    template<typename T> void addSystem(T&&);
    void removeSystem(size_t index);
};
```

**特点**：

- ✅ 使用 `entt::poly<ISystem>` 实现多态
- ✅ 支持动态添加/移除系统
- ✅ 统一的系统接口调用
- ✅ 类型安全的系统访问

**系统接口**：

```cpp
struct ISystem : entt::type_list<> {
    template <typename Base>
    struct type : Base {
        void registerHandlers();
        void unregisterHandlers();
        void update();
    };
};

// 系统实现
class MySystem : public EnableRegister<MySystem> {
public:
    void registerHandlersImpl() { /* ... */ }
    void unregisterHandlersImpl() { /* ... */ }
    void updateImpl() { /* ... */ }
};
```

---

### 5. Application（应用程序）

**职责**：组合所有组件并协调工作

```cpp
class Application {
private:
    GraphicsContext m_graphicsContext;
    std::unique_ptr<ImguiContext> m_imguiContext;
    EventLoop m_eventLoop;
    SystemManager m_systems;
    entt::entity m_rootEntity;
    
public:
    Application() {
        // 1. 初始化 SDL
        SDL_Init(...);
        
        // 2. 创建窗口和渲染器
        window = SDL_CreateWindow(...);
        renderer = SDL_CreateRenderer(...);
        
        // 3. 初始化各组件
        m_graphicsContext.initialize(window, renderer, 800, 600);
        m_imguiContext = std::make_unique<ImguiContext>(window, renderer);
        
        // 4. 依赖注入
        m_systems.setGraphicsContext(&m_graphicsContext);
        
        // 5. 设置 UI
        setupRootEntity();
        setupUI();  // 虚函数，子类实现
    }
    
    virtual ~Application() {
        m_eventLoop.quit();
        m_systems.unregisterAllHandlers();
        m_imguiContext.reset();  // 自动清理 ImGui
        // 销毁 SDL 资源
    }
    
    void run() {
        while (running) {
            // 1. 时间更新
            // 2. 处理事件
            // 3. 更新系统
            // 4. 渲染
        }
    }
    
protected:
    virtual void setupUI() = 0;
};
```

---

## 完整初始化流程

```
1. SDL_Init()
   ↓
2. SDL_CreateWindow() / SDL_CreateRenderer()
   ↓
3. GraphicsContext.initialize()
   ↓
4. ImguiContext 构造
   ├─ IMGUI_CHECKVERSION()
   ├─ ImGui::CreateContext()
   ├─ ImGui_ImplSDL3_InitForSDLRenderer()
   └─ ImGui_ImplSDLRenderer3_Init()
   ↓
5. SystemManager.setGraphicsContext()
   └─ RenderSystem.setGraphicsContext()
   ↓
6. SystemManager 构造所有系统
   ├─ InteractionSystem
   ├─ AnimationSystem
   ├─ LayoutSystem
   ├─ RenderSystem
   └─ WindowSystem
   ↓
7. SystemManager.registerAllHandlers()
   ↓
8. setupRootEntity()
   ↓
9. setupUI() (用户自定义)
```

---

## 为什么这样设计？

### ✅ 组件化封装

| 组件 | 职责 | 优势 |
|------|------|------|
| GraphicsContext | SDL 资源 | 统一管理，易于传递 |
| ImguiContext | ImGui 生命周期 | RAII，自动清理 |
| EventLoop | 异步调度 | 支持未来扩展 |
| SystemManager | 系统管理 | 动态、可扩展 |

### ✅ 依赖注入 vs 单例

| 特性 | 单例 | 依赖注入 |
|------|------|---------|
| 测试性 | ❌ 难以 mock | ✅ 易于测试 |
| 多实例 | ❌ 不支持 | ✅ 支持多窗口 |
| 耦合度 | ❌ 高耦合 | ✅ 低耦合 |
| 生命周期 | ❌ 隐式 | ✅ 显式清晰 |

### ✅ entt::poly 动态系统管理

```cpp
// 旧方式：硬编码系统
class SystemManager {
    InteractionSystem m_interaction;
    AnimationSystem m_animation;
    // ... 每个系统一个成员变量
};

// 新方式：动态管理
class SystemManager {
    std::vector<entt::poly<ISystem>> m_systems;
    
    // 可以动态添加/移除
    void addSystem(auto&& system) {
        m_systems.emplace_back(std::forward<decltype(system)>(system));
    }
};
```

**优势**：

- ✅ 更灵活的系统管理
- ✅ 统一的接口调用
- ✅ 易于扩展新系统
- ✅ 支持运行时系统切换

---

## 使用示例

```cpp
// 1. 创建自定义应用
class GameUI : public ui::Application {
protected:
    void setupUI() override {
        auto& registry = utils::Registry::getInstance();
        
        // 创建按钮
        auto button = registry.create();
        registry.emplace<ui::components::Position>(button, ImVec2{100, 100});
        registry.emplace<ui::components::Size>(button, ImVec2{200, 50});
        registry.emplace<ui::components::Background>(button);
        
        // 设置点击回调
        auto& clickable = registry.emplace<ui::components::Clickable>(button);
        clickable.onClick = [](entt::entity e) {
            // 处理点击
        };
        
        // 可选：使用事件循环投递异步任务
        getEventLoop().invoke([this]() {
            // 异步加载资源等
        });
    }
};

// 2. 运行应用
int main() {
    try {
        GameUI app;
        app.run();  // 进入主循环
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
```

---

## 总结

### 核心特性

- ✅ **GraphicsContext**：图形资源封装，依赖注入
- ✅ **ImguiContext**：ImGui 生命周期自动管理
- ✅ **EventLoop**：异步事件调度基础设施
- ✅ **SystemManager**：entt::poly 动态系统管理
- ✅ **Application**：组合所有组件的容器

### 设计优势

- 高内聚、低耦合
- 易于测试和维护
- 支持未来扩展（多窗口、异步 UI、自定义系统）
- 资源管理安全可靠（RAII）
- 灵活的系统架构（entt::poly）
