
# PestManKill — AI 代码代理简明指南（精简合并版）

此指南为 AI 编程代理提供立即上手的可执行信息：项目架构、关键文件、构建/测试命令、常见约定与注意事项。

概览
- 模块划分：`src/client/`（UI、SDL3/ImGui）、`src/server/`（权威游戏逻辑 + 网络）、`src/shared/`（协议与公用类型）。
- 核心技术：EnTT（ECS）、ASIO + C++20 协程（awaitable）、spdlog、CMake + Ninja。

快速入口（常查文件）
- 服务端：[src/server/main.cpp](src/server/main.cpp#L1)
- 客户端：[src/client/main.cpp](src/client/main.cpp#L1)
- 游戏上下文：[src/server/context/GameContext.h](src/server/context/GameContext.h#L1)
- 网络管理：[src/server/net/NetWorkManager.h](src/server/net/NetWorkManager.h#L1)
- 会话管理：[src/server/net/ClientSessionManager.h](src/server/net/ClientSessionManager.h#L1)
- 协议/消息：[src/shared/messages/](src/shared/messages/)
- 客户端主界面：[src/client/view/MainWindow.h](src/client/view/MainWindow.h#L1)

构建 / 运行（Windows PowerShell）
- 生成并构建：
	- `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`
	- `cmake --build build --config Release`
- 运行示例：
	- `.uild\src\client\Release\PestManKillClient.exe`
	- `.uild\src\server\Release\PestManKillServer.exe`
- 测试：构建后用 `ctest -C Release` 或直接运行测试可执行文件（`test/unittest/`）。

项目约定（重要）
- 头文件优先：大多数实现放在 `.h` 中，尽量不要新增不必要的 `.cpp` 文件。
- 日志：始终使用 `m_context->logger->info/warn/error()`（spdlog），避免 `std::cout`。
- 包含路径：使用项目根相对 `#include "src/..."` 风格。
- 线程/锁：网络与重试状态（如 `m_pendingAcks`）由已有 mutex 保护，复用现有同步策略。

常见修改模式（可直接执行）
- 修改系统逻辑：编辑 `src/server/systems/` 下的头文件并注册到 `dispatcher.sink<EventType>()`。
- 添加/变更消息：更新 `src/shared/messages/` 的定义并在 `NetWorkManager` 中处理序列化/反序列化。
- 会话调试：使用 `ClientSessionManager::registerClient/updateHeartbeat/getPlayerEntity` 定位 endpoint↔entity 的映射问题（示例调用：`m_sessionManager->registerClient(endpoint, playerEntity, playerName);`）。
- 资源嵌入：修改 `CMakeLists.txt` 中 `ASSETS` 列表并查看 `cmake/embed_resource.cmake` 生成的 `build/embedded/` 头文件。

不要修改的区域
- 不要改动 `third_party/` 里的代码。
- 避免全局可变状态或跨帧持有 `entt::entity`（会导致悬挂/并发问题）。

快速查找建议
- 查找网络相关逻辑：`src/server/net/`、`m_pendingAcks`、`ClientSessionManager`。
- 查找事件流：`dispatcher` 使用点在 `src/server/context/GameContext.h` 与各系统头文件中。

调试与静态分析
- 使用 `build/compile_commands.json` 支持 clangd。
- 启用 clang-tidy：`cmake -B build -DENABLE_CLANG_TIDY=ON`。

如果需要我可以：
- 增补某个子系统的可执行修改示例（例如：`ClientSessionManager` 或 `NetWorkManager`）。
- 或把本文件扩展为英/中双语、包含更多代码片段的版本。

请审阅并指出需要补充或有歧义的部分。
