cmake_minimum_required(VERSION 3.15)

add_library(net STATIC
    transport/AsioUdpTransport.cpp
    Session/KcpSession.cpp
    App/Server.cpp
)



target_include_directories(net PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(net PUBLIC
    kcp
    abseil-cpp
)

target_link_libraries(net PUBLIC asio::asio)
