add_library(blueduck_api STATIC
    controllers/ProjectController.cpp
    controllers/VersionController.cpp
    controllers/ReportController.cpp
    controllers/CveSourceController.cpp
    controllers/CpeMappingController.cpp
)

target_link_libraries(blueduck_api
    PUBLIC
        blueduck_db
        blueduck_git
        blueduck_report
        Drogon::Drogon
)

target_include_directories(blueduck_api PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
