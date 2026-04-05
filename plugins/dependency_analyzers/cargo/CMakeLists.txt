add_blueduck_plugin(blueduck_plugin_cargo
    TYPE dependency_analyzer
    SOURCES
        CargoAnalyzer.cpp
    INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}
    LIBRARIES
)
