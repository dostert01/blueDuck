add_library(blueduck_compiler_options INTERFACE)

target_compile_options(blueduck_compiler_options INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(blueduck_compiler_options INTERFACE -O3)
endif()
