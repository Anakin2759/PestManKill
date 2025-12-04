# PestManKill - AI Coding Agent Instructions

## Project Overview
PestManKill is a multiplayer card game inspired by "San Guo Sha" (Three Kingdoms Kill), built as a client-server architecture using modern C++23. It's a learning project demonstrating advanced C++ patterns, ECS architecture, and asynchronous networking.

## Architecture

### Client-Server Split
- **Server** (`src/server/`): Authoritative game logic, ECS systems, UDP networking
- **Client** (`src/client/`): SDL3/ImGui rendering, UI framework, network client
- **Shared** (`src/shared/`): Message protocols, common types, utilities

### Core Patterns

#### Entity-Component-System (EnTT)
All game logic uses EnTT registry and dispatcher:
```cpp
// Components live in src/server/components/ - pure data structs
struct Attributes { int maxHp; int currentHp; int attackRange; };
struct Deck { std::vector<entt::entity> drawPile; std::vector<entt::entity> discardPile; };

// Systems live in src/server/systems/ - behavior logic
class DamageSystem {
    void registerEvents() { m_context->dispatcher.sink<events::DamageEvent>().connect<&DamageSystem::onDamage>(this); }
};
```

**Key insight**: Systems are polymorphic via `entt::poly<ISystem>` but no ISystem.h exists - check `SystemManager.h` for the pattern. Systems manage `GameContext` (registry + dispatcher + logger + threadPool).

#### Async Networking (ASIO + C++23 Coroutines)
Network layer uses `asio::awaitable` coroutines with thread pool execution:
```cpp
// See src/server/net/NetWorkManager.h
asio::awaitable<bool> sendReliablePacket(endpoint, type, payload, maxRetries, timeout);
asio::awaitable<void> receiveLoop(); // Event-driven ACK handling
```

**Critical**: 
- All async operations use `asio::co_spawn` with `m_threadPool->get_executor()`
- ACK mechanism tracked per-endpoint with `std::unordered_map<AckKey, timer>`
- Use `asio::detached` for fire-and-forget coroutines
- Avoid `&&` in PowerShell commands - use `;` for command chaining

#### Client Session Management
Server tracks multiple clients via `ClientSessionManager` (see `src/server/net/ClientSessionManager.h`):
```cpp
// UDP endpoint ↔ entt::entity bidirectional mapping
m_sessionManager->registerClient(endpoint, playerEntity, playerName);
auto entity = m_sessionManager->getPlayerEntity(endpoint);  // endpoint → entity
auto endpoint = m_sessionManager->getEndpoint(entity);       // entity → endpoint

// Broadcasting
auto endpoints = m_sessionManager->getAllEndpoints();
m_sessionManager->checkTimeouts(); // Returns timed-out entities for cleanup
```

**Key patterns**:
- Update heartbeat on every packet: `updateHeartbeat(endpoint)`
- Always check `getPlayerEntity()` returns `std::optional` - may be empty
- Default timeout: 30s (configurable via `HEARTBEAT_TIMEOUT`)
- Thread-safe - uses internal mutex for all operations

**Disconnect handling** (important for game sessions):
- Timeouts **mark** clients as disconnected (`isDisconnected = true`), **don't remove** them
- Allows reconnection: `reconnectClient(endpoint)` clears disconnect flag
- Game entities preserved until explicit cleanup
- Use `getAllEndpoints(excludeDisconnected=true)` to broadcast only to online players
- Call `clearAllSessions()` at game end to cleanup all sessions at once
- See `docs/Disconnect_Reconnect_Guide.md` for reconnection patterns

#### Custom UI Framework (Qt-inspired)
Client uses a homegrown widget system (`src/client/ui/`) wrapping SDL3 + ImGui:
```cpp
// See src/client/view/MainWindow.h
class MainWindow : public ui::Application {
    void setupUI() override {
        auto layout = std::make_shared<ui::VBoxLayout>();
        layout->addWidget(widget, stretchFactor);
    }
};
```

Widgets use layout managers (HBoxLayout/VBoxLayout) for responsive sizing. All rendering happens in `Application::run()` loop.

## Build System

### CMake Workflow
```powershell
# Configure (from project root)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Run executables
.\build\src\client\Release\PestManKillClient.exe
.\build\src\server\Release\PestManKillServer.exe
```

### Resource Embedding
Assets are embedded as C arrays at build time:
1. `cmake/embed_resource.cmake` converts files to hex arrays
2. Headers generated to `build/embedded/` (e.g., `1.jpeg.h`)
3. Reference in code: `#include <1.jpeg.h>` then access `_1_jpeg` array

**When adding assets**: Update `ASSETS` list in root `CMakeLists.txt` (line 58).

### Static Analysis
Enable clang-tidy: `cmake -B build -DENABLE_CLANG_TIDY=ON`
Config in `.clang-tidy` - strict checks enabled (see lines 1-30).

## Conventions

### File Organization
- **Headers-only**: Most code is header-only (`.h` files contain full implementations)
- **CMakeLists.txt**: Each executable has its own (`src/client/CMakeLists.txt`, `src/server/CMakeLists.txt`)
- **Namespaces**: Events use `namespace events {}`, avoid elsewhere except `ui::`

### Coding Style
- **C++23 features**: Use ranges, coroutines, `std::expected` where appropriate
- **Magic numbers**: NOLINT comments required (see `MainWindow.h` line 27)
- **Smart pointers**: Use `std::shared_ptr` for widgets/layouts, `std::unique_ptr` for ownership
- **Logging**: Always use `m_context->logger->info/warn/error()` (spdlog), never `std::cout`
- **Includes**: Absolute paths from project root: `#include "src/server/context/GameContext.h"`

### Common Pitfalls
1. **Don't create `.cpp` files** unless absolutely necessary (project is header-heavy)
2. **Thread safety**: Protect `m_pendingAcks` with `std::mutex` in networking code
3. **Entity lifetime**: Never store `entt::entity` across frame boundaries without validation
4. **ASIO coroutines**: Capture `shared_from_this()` in lambdas, not `this`

## Key Dependencies
- **EnTT**: Registry (`m_context->registry`), dispatcher (event bus), `entt::poly` for type erasure
- **ASIO**: Coroutines (`co_await`), thread pool, UDP sockets
- **SDL3**: Window/renderer creation, event loop
- **ImGui**: Immediate mode UI (wrapped by custom `ui::` classes)
- **spdlog**: Structured logging (header-only mode)
- **nlohmann/json**: Character/card data loading from `resource/`
- **Abseil**: Containers (`flat_hash_map`, `InlinedVector`)

## Testing & Debugging
- **Logs**: Check `logs/` directory for rotating log files
- **Compile commands**: `build/compile_commands.json` for clangd/IDE
- **Third-party**: Never modify `third_party/` contents directly

## Game Logic Flow
1. `GameFlowSystem` drives turn phases via state machine (see `TurnPhase` enum in `src/shared/common/Common.h`)
2. Phase transitions trigger events → Systems handle via `dispatcher.sink<EventType>()`
3. Example: `events::GameStart` → `GameFlowSystem::onGameStart()` → setup players
4. Networking: Client sends `UseCardMessage` → Server validates → broadcasts result

## Questions to Clarify
- Is server `main.cpp` intended to be minimal (currently just prints "Server is running...")?
- Should `ISystem` interface be formally defined or stay as polymorphic duck-typing?
- Resource JSON schema for characters (`Pest.json`) - are there validation requirements?
