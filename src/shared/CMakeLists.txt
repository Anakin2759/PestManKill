cmake_minimum_required(VERSION 3.20)

# Shared 模块 (INTERFACE 库，纯头文件)
add_library(shared INTERFACE)

# 头文件目录
target_include_directories(shared INTERFACE
    ${CMAKE_SOURCE_DIR}
)

# 依赖项 (shared 模块需要 nlohmann_json 和 entt)
target_link_libraries(shared INTERFACE
    nlohmann_json::nlohmann_json
    EnTT::EnTT
)

# C++23 标准
target_compile_features(shared INTERFACE cxx_std_23)

# 显式编译选项以确保 MSVC/clang-cl 正确识别 C++23
target_compile_options(shared INTERFACE
    $<$<CXX_COMPILER_ID:GNU>:-std=c++23>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>>:-std=c++23>
    $<$<CXX_COMPILER_ID:MSVC>:/std:c++latest>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>:/std:c++latest>
)
