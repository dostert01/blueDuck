add_blueduck_plugin(blueduck_plugin_python
    TYPE dependency_analyzer
    SOURCES
        PythonAnalyzer.cpp
    INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}
    LIBRARIES
)
