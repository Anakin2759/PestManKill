cmake_minimum_required(VERSION 3.15)

add_library(net STATIC
    transport/AsioUdpTransport.cpp
    Session/KcpSession.cpp
    App/Server.cpp
    App/KcpEndpoint.cpp
    App/Client.cpp
)

target_compile_options(net PRIVATE
    # GCC
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g3 -fno-omit-frame-pointer -fno-inline>
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG>

    # Clang
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Debug>>:-Wall -Wextra -Wpedantic -O0 -g3 -fno-omit -frame-pointer -fno-inline>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:Release>>:-Wall -O3 -DNDEBUG>

    # MSVC
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/W4 /Od /Zi /EHsc>
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/O2 /DNDEBUG /EHsc>

    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Debug>>:/EHsc /Zi /W4>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_FRONTEND_VARIANT:MSVC>,$<CONFIG:Release>>:/EHsc /O2 /DNDEBUG>

)


target_include_directories(net PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(net PUBLIC
    kcp
    asio::asio
    nlohmann_json::nlohmann_json
)
