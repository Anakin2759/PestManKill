#  ui 模块 CMake 配置文件
cmake_minimum_required(VERSION 3.20)
OPTION(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
OPTION(BUILD_STATIC_LIBS "Build using static libraries" ON)

set(UI_SOURCES 
    core/Application.cpp
    core/EventLoop.cpp
    core/SystemManager.cpp
    api/Size.cpp
    api/Visibility.cpp
    api/Layout.cpp
    api/Text.cpp
    api/Animation.cpp
    api/Hierarchy.cpp
    api/Utils.cpp
    api/Factory.cpp
)
set(UI_HEADERS
    # Components
    common/Components.hpp
    common/Policies.hpp
    common/Tags.hpp
    common/Events.hpp
    common/Types.hpp
    
    # Systems
    systems/InteractionSystem.hpp
    systems/TweenSystem.hpp
    systems/LayoutSystem.hpp
    systems/RenderSystem.hpp
    systems/StateSystem.hpp
    
    # UI Core
    core/Application.hpp
    core/SystemManager.hpp
    core/EventLoop.hpp
    core/TaskChain.hpp

    singleton/Registry.hpp
    singleton/SingletonBase.hpp
    singleton/Logger.hpp
    
    traits/ComponentsTraits.hpp
    traits/PoliciesTraits.hpp

    # Interface
    interface/Isystem.hpp
    ui/ui.hpp
)
# UI 模块库定义
add_library(ui STATIC ${UI_SOURCES} ${UI_HEADERS})

# 头文件目录
target_include_directories(ui
    PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
)

cmrc_add_resource_library(ui_fonts
    ALIAS fonts              # 在 C++ 中调用的别名
    NAMESPACE ui_fonts          # C++ 命名空间
    assets/fonts/NotoSansSC-VariableFont_wght.ttf
    assets/shader/vert.spv
    assets/shader/frag.spv
    assets/shader/vert.dxil
    assets/shader/frag.dxil
)

# 依赖项
target_link_libraries(ui PRIVATE
    SDL3::SDL3
    EnTT::EnTT
    asio::asio
    yogacore
    SDL3_ttf::SDL3_ttf
    eigen
    ui_fonts
    spdlog::spdlog_header_only
)
target_compile_features(ui PUBLIC cxx_std_23)

target_compile_options(ui PUBLIC
    # GCC
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g >
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG >

    # Clang (non-MSVC frontend)
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g >
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG >

    # MSVC
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/W4 /Od /Zi /EHsc >
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/O2 /DNDEBUG /EHsc >

    # Clang-cl (MSVC frontend)
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Debug>>:/EHsc /Zi /W4 >
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Release>>:/EHsc /O2 /DNDEBUG >

)



