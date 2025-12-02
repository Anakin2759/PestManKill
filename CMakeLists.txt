cmake_minimum_required(VERSION 3.31)
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

# ==================== 资源嵌入 ====================
# 必须在 add_subdirectory(src/client) 之前处理

# 生成资源头文件目录
set(EMBED_DIR ${CMAKE_CURRENT_BINARY_DIR}/embedded)
file(MAKE_DIRECTORY ${EMBED_DIR})

# 要嵌入的资源
set(ASSETS
    ${CMAKE_SOURCE_DIR}/assets/1.jpeg
)

# 存放生成的头文件
set(EMBED_HEADERS "")

# 自动嵌入函数 - 使用 CMake 脚本生成 C 数组头文件
function(embed_resource INPUT_FILE OUTPUT_HEADER VAR_NAME)
    add_custom_command(
        OUTPUT ${OUTPUT_HEADER}
        COMMAND ${CMAKE_COMMAND} 
            -DINPUT_FILE=${INPUT_FILE}
            -DOUTPUT_HEADER=${OUTPUT_HEADER}
            -DVAR_NAME=${VAR_NAME}
            -P ${CMAKE_SOURCE_DIR}/cmake/embed_resource.cmake
        DEPENDS ${INPUT_FILE}
        COMMENT "Embedding resource: ${INPUT_FILE}"
        VERBATIM
    )
endfunction()

foreach(asset ${ASSETS})
    get_filename_component(name ${asset} NAME)
    get_filename_component(name_we ${asset} NAME_WE)
    set(header ${EMBED_DIR}/${name}.h)
    # 生成合法的 C 变量名（替换特殊字符为下划线）
    string(REGEX REPLACE "[^a-zA-Z0-9]" "_" var_name ${name})
    embed_resource(${asset} ${header} ${var_name})
    list(APPEND EMBED_HEADERS ${header})
endforeach()

# 创建一个自定义目标来确保资源被生成
add_custom_target(EmbeddedResources DEPENDS ${EMBED_HEADERS})

# ==================== 子项目 ====================

add_subdirectory(src/client)
add_subdirectory(src/server)