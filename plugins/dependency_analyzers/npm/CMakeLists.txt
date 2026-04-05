add_blueduck_plugin(blueduck_plugin_npm
    TYPE dependency_analyzer
    SOURCES
        NpmAnalyzer.cpp
    INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}
    LIBRARIES
        nlohmann_json::nlohmann_json
)
