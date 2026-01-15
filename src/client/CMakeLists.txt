cmake_minimum_required(VERSION 3.20)

set(CLIENT_NAME PestManKillClient)

# 客户端源文件
set(CLIENT_SOURCES
    main.cpp
)

# 客户端头文件
set(CLIENT_HEADERS
    )

# 创建客户端可执行文件
add_executable(${CLIENT_NAME} ${CLIENT_SOURCES} ${CLIENT_HEADERS})
# 编译选项
target_compile_options(${CLIENT_NAME} PRIVATE
    # GCC
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g3 -fno-omit-frame-pointer -fno-inline -std=c++23>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG -std=c++23>

    # Clang (non-MSVC frontend)
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g3 -fno-omit-frame-pointer -fno-inline -std=c++23>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<NOT:$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG -std=c++23>

    # MSVC
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/W4 /Od /Zi /EHsc /utf-8 /std:c++23 /bigobj>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/O2 /DNDEBUG /EHsc /utf-8 /std:c++23 /bigobj>

    # Clang-cl (MSVC frontend)
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Debug>>:/EHsc /Zi /W4 /std:c++23 /bigobj>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Release>>:/EHsc /O2 /DNDEBUG /std:c++23 /bigobj>
)

# 包含目录
target_include_directories(${CLIENT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party/stb
)

# 链接库
target_link_libraries(${CLIENT_NAME} PRIVATE 
    # 核心依赖
    mimalloc-static 
    utils
    shared
    nlohmann_json::nlohmann_json
    
    # 网络模块
    net
    asio::asio
    kcp
    
    # UI 模块
    ui
    SDL3::SDL3
    
    # Abseil
    absl::base 
    absl::memory 
    absl::strings
    absl::hash
    absl::flat_hash_map
    absl::raw_hash_set
    
)

# 设置输出目录
set_target_properties(${CLIENT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# C++23 标准
target_compile_features(${CLIENT_NAME} PRIVATE cxx_std_23)





