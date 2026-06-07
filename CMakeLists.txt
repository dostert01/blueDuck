cmake_minimum_required(VERSION 3.25)
project(blueDuck VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(CompilerOptions)
include(PluginHelpers)
include(FetchContent)
set(BUILD_CTL      OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_DOC      OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    drogon
    GIT_REPOSITORY https://github.com/drogonframework/drogon.git
    GIT_TAG        v1.9.10
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Drogon)  # <-- was: Drogon
FetchContent_Declare(
    dean
    GIT_REPOSITORY https://github.com/dostert01/dean.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(dean)
find_package(LibGit2 1.7 REQUIRED)
find_package(CURL 8.5 REQUIRED)
find_package(OpenSSL 3.0 REQUIRED)
find_package(PostgreSQL 16 REQUIRED)
find_package(nlohmann_json 3.11 REQUIRED)
find_package(pugixml 1.14 REQUIRED)
find_package(libzip 1.7 REQUIRED)
add_subdirectory(interfaces)
add_subdirectory(src)
add_subdirectory(plugins)
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()