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

find_package(Drogon 1.9.10 REQUIRED)
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
