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
