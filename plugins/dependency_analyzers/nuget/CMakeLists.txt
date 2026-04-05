add_blueduck_plugin(blueduck_plugin_nuget
    TYPE dependency_analyzer
    SOURCES
        NugetAnalyzer.cpp
    INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}
    LIBRARIES
        pugixml::pugixml
)
