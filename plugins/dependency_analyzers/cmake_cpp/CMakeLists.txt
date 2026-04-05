add_blueduck_plugin(blueduck_plugin_cmake_cpp
    TYPE dependency_analyzer
    SOURCES
        CmakeAnalyzer.cpp
    INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}
    LIBRARIES
)
