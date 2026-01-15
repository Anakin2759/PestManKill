cmake_minimum_required(VERSION 3.15)

add_library(net STATIC
    transport/AsioUdpTransport.cpp
    Session/KcpSession.cpp
    App/Server.cpp
    App/KcpEndpoint.cpp
    App/Client.cpp
)
# C++23 标准
target_compile_options(net INTERFACE
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


target_include_directories(net PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(net PUBLIC
    kcp
    asio::asio
    nlohmann_json::nlohmann_json
)
# 导出头文件列表（用于 IDE 项目视图）
set(NET_HEADERS
    # Components
    transport/AsioUdpTransport.h
    Session/KcpSession.h
    App/Server.h
    App/KcpEndpoint.h
    App/Client.h
)

# 添加到目标源文件（仅用于 IDE 显示）
target_sources(net INTERFACE ${NET_HEADERS})