cmake_minimum_required(VERSION 4.0)
project(PestMan LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) #生成compile_commands.json文件
set(CMAKE_CXX_EXTENSIONS OFF)


# Optionally enable clang-tidy static analysis. Can be overridden by passing
# -DENABLE_CLANG_TIDY=OFF to CMake command line.
option(ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" ON)

# Try to locate clang-tidy (check several common names)
find_program(CLANG_TIDY_EXE
    NAMES clang-tidy
)


if (ENABLE_CLANG_TIDY)
    if (CLANG_TIDY_EXE)
        message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
        # Default checks can be customized by setting CLANG_TIDY_OPTIONS
        set(CLANG_TIDY_OPTIONS "--checks=*,-clang-analyzer-alpha*")
        # Apply globally so all targets are analyzed; users can override with
        # -DCMAKE_CXX_CLANG_TIDY="<path>;[options]"
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};${CLANG_TIDY_OPTIONS}")
    else()
        message(STATUS "ENABLE_CLANG_TIDY is ON but clang-tidy executable not found.")
        message(STATUS "Install clang-tidy or set CLANG_TIDY_EXE to its path to enable.")
    endif()
else()
    message(STATUS "clang-tidy integration disabled (ENABLE_CLANG_TIDY=OFF)")
endif()

# Build SDL_image as static library
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/mimalloc)
add_subdirectory(third_party/entt)
add_subdirectory(third_party/json)
add_subdirectory(third_party/spdlog)
add_subdirectory(third_party/asio)
add_subdirectory(third_party/SDL)

add_subdirectory(third_party/stb)

add_subdirectory(third_party/imgui)
add_subdirectory(third_party/abseil-cpp)



add_subdirectory(src/client)
add_subdirectory(src/server)

# 自动嵌入函数
function(embed_resource INPUT_FILE OUTPUT_HEADER)
    add_custom_command(
        OUTPUT  ${OUTPUT_HEADER}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/embedded
        COMMAND xxd -i ${INPUT_FILE} > ${OUTPUT_HEADER}
        DEPENDS ${INPUT_FILE}
        COMMENT "Embedding resource: ${INPUT_FILE}"
    )
endfunction()

# 生成资源头文件目录
set(EMBED_DIR ${CMAKE_CURRENT_BINARY_DIR}/embedded)

# 要嵌入的资源
set(ASSETS
    ${CMAKE_SOURCE_DIR}/assets/icon.png
    ${CMAKE_SOURCE_DIR}/assets/card_back.png
)

# 存放生成的头文件
set(EMBED_HEADERS "")

foreach(asset ${ASSETS})
    get_filename_component(name ${asset} NAME)
    set(header ${EMBED_DIR}/${name}.h)
    embed_resource(${asset} ${header})
    list(APPEND EMBED_HEADERS ${header})
endforeach()