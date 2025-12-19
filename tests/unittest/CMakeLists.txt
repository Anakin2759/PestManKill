#include(FetchContent)

# FetchContent_Declare(
#     googletest
#     URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
# )

# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(googletest)

find_package(GTest REQUIRED)

# ----------------------------
# Net module tests
# ----------------------------
add_executable(net_tests
    net/test_kcp_session.cpp
    net/test_kcp_server.cpp
    net/test_frame_codec.cpp
    net/test_transport.cpp
    net/test_peek_conv.cpp
)

target_include_directories(net_tests PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(net_tests PRIVATE
    net
    GTest::gtest
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(net_tests)
