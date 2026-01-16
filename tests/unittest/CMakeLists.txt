#include(FetchContent)

# FetchContent_Declare(
#     googletest
#     URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
# )

# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# FetchContent_MakeAvailable(googletest)



# ----------------------------
# Subdirectories for each module
# ----------------------------
add_subdirectory(net)
add_subdirectory(ui)
