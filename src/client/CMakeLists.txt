cmake_minimum_required(VERSION 3.20)

set(CLIENT_NAME PestManKillClient)

# 客户端源文件
set(CLIENT_SOURCES
    main.cpp
    SceneImpl.cpp
)

# 客户端头文件
set(CLIENT_HEADERS
    GameClient.h
    SceneManager.h
)

# 创建客户端可执行文件
add_executable(${CLIENT_NAME} ${CLIENT_SOURCES} ${CLIENT_HEADERS})

# 编译选项
target_compile_options(${CLIENT_NAME} PRIVATE
    # GCC
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g3 -fno-omit-frame-pointer -fno-inline>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG>

    # Clang
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g3 -fno-omit-frame-pointer -fno-inline>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG>

    # MSVC
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/W4 /Od /Zi /EHsc /utf-8>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/O2 /DNDEBUG /EHsc /utf-8>

    # Clang-CL
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Debug>>:/EHsc /Zi /W4>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Release>>:/EHsc /O2 /DNDEBUG>
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
    imgui::imgui
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

# ========== 测试可执行文件（可选） ==========
set(TEST_CLIENT_NAME PestManKillClientTest)

if(BUILD_TESTING)
    add_executable(${TEST_CLIENT_NAME} main.cpp SceneImpl.cpp)
    
    target_compile_options(${TEST_CLIENT_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /Od /Zi /EHsc /utf-8>
    )
    
    target_include_directories(${TEST_CLIENT_NAME} PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/third_party/stb
    )
    
    target_link_libraries(${TEST_CLIENT_NAME} PRIVATE 
        utils
        net
        ui
        asio::asio
        imgui::imgui
        SDL3::SDL3
    )
    
    target_compile_features(${TEST_CLIENT_NAME} PRIVATE cxx_std_23)
endif()



