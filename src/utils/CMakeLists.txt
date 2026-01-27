cmake_minimum_required(VERSION 3.20)

# Utils 工具库（Header-Only）
add_library(utils INTERFACE)

# 头文件目录
target_include_directories(utils INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

# 依赖项
target_link_libraries(utils INTERFACE
    asio::asio
    EnTT::EnTT
    spdlog::spdlog_header_only

)
target_compile_definitions(
    utils
    INTERFACE
    ASIO_NO_DEPRECATED
)


# 显式编译选项以确保 MSVC/clang-cl 正确识别 C++23
target_compile_options(utils INTERFACE
    $<$<CXX_COMPILER_ID:GNU>:-std=c++23>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>>:-std=c++23>
    $<$<CXX_COMPILER_ID:MSVC>:/std:c++latest>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>:/std:c++latest>
)

# 导出头文件列表（用于 IDE 项目视图）
set(UTILS_HEADERS
    Dispatcher.h
    Logger.h
    Registry.h
    ThreadPool.h
    utils.h
    Functions.h
)

# 添加到 target 的 SOURCES（仅用于 IDE 显示）
target_sources(utils INTERFACE
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
    FILES ${UTILS_HEADERS}
)

