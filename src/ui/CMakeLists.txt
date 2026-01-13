#  ui 模块 CMake 配置文件
cmake_minimum_required(VERSION 3.20)

# UI 模块库定义
add_library(ui INTERFACE)

# 头文件目录
target_include_directories(ui INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
cmrc_add_resource_library(ui_fonts
    ALIAS fonts              # 在 C++ 中调用的别名
    NAMESPACE ui_fonts          # C++ 命名空间
    SourceHanSans-VF.ttf.ttc
)

# 依赖项
target_link_libraries(ui INTERFACE
    utils
    imgui::imgui
    SDL3::SDL3
    EnTT::EnTT
    asio::asio
    yogacore
    ui_fonts
)



# C++23 标准
target_compile_features(ui INTERFACE cxx_std_23)


target_compile_options(ui INTERFACE
    # GCC
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g -std=c++23>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG -std=c++23>

    # Clang (non-MSVC frontend)
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g -std=c++23>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG -std=c++23>

    # MSVC
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/W4 /Od /Zi /EHsc /std:c++latest>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/O2 /DNDEBUG /EHsc /std:c++latest>

    # Clang-cl (MSVC frontend)
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Debug>>:/EHsc /Zi /W4 /std:c++latest>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Release>>:/EHsc /O2 /DNDEBUG /std:c++latest>

)
# 导出头文件列表（用于 IDE 项目视图）
set(UI_HEADERS
    # Components
    components/Components.h
    components/Policies.h
    components/Tags.h
    components/Events.h
    
    # Systems
    systems/InteractionSystem.h
    systems/AnimationSystem.h
    systems/LayoutSystem.h
    systems/RenderSystem.h
    systems/WindowsSystem.h
    
    # UI Core
    core/Application.h
    core/Factory.h
    core/helper.h
    core/SystemManager.h
    core/EventLoop.h
    core/ImguiContext.h

    # Interface
    interface/Isystem.h
)

# 添加到目标源文件（仅用于 IDE 显示）
target_sources(ui INTERFACE ${UI_HEADERS})

# 安装规则（可选）
install(DIRECTORY components systems core interface
    DESTINATION include/PestManKill/ui
    FILES_MATCHING PATTERN "*.h"
)
