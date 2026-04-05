add_library(blueduck_core STATIC
    Config.cpp
    Application.cpp
    PluginLoader.cpp
    AnalysisService.cpp
)

target_link_libraries(blueduck_core
    PUBLIC
        blueduck_interfaces
        blueduck_db
        blueduck_git
        Drogon::Drogon
)

target_include_directories(blueduck_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
