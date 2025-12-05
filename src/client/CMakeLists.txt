set(EXET_NAME PestManKillClient)

# 源文件
set(SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)

add_executable(${EXET_NAME} ${SOURCES})


target_compile_options(${EXET_NAME} PRIVATE
    # GCC
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG>

    # Clang
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG>

    # MSVC
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/W4 /Od /Zi /EHsc>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/O2 /DNDEBUG /EHsc>

    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Debug>>:/EHsc /Zi /W4>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Release>>:/EHsc /O2 /DNDEBUG>

)
target_include_directories(${EXET_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/embedded  # 添加嵌入资源头文件目录
)

target_link_libraries(${EXET_NAME} PRIVATE 
    mimalloc-static 
    asio::asio 
    spdlog::spdlog_header_only  
    SDL3::SDL3 
    stb::stb
    imgui::imgui  
    EnTT::EnTT 
    nlohmann_json::nlohmann_json 
    absl::base 
    absl::memory 
    absl::strings
    absl::hash
    absl::flat_hash_map
    absl::raw_hash_set)

# 测试可执行文件 - 心跳包通信测试（无 GUI）
set(TEST_EXET_NAME PestManKillClientTest)
set(TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/main_test.cpp"
)

add_executable(${TEST_EXET_NAME} ${TEST_SOURCES})

target_compile_options(${TEST_EXET_NAME} PRIVATE
    # GCC
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG>

    # Clang
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG>

    # MSVC
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/W4 /Od /Zi /EHsc>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/O2 /DNDEBUG /EHsc>

    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Debug>>:/EHsc /Zi /W4>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Release>>:/EHsc /O2 /DNDEBUG>

)

target_include_directories(${TEST_EXET_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(${TEST_EXET_NAME} PRIVATE 
    mimalloc-static 
    asio::asio 
    spdlog::spdlog_header_only  
    EnTT::EnTT 
    nlohmann_json::nlohmann_json 
    absl::base 
    absl::memory 
    absl::strings
    absl::hash
    absl::flat_hash_map
    absl::raw_hash_set)
