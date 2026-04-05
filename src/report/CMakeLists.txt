add_library(blueduck_report STATIC
    ReportEngine.cpp
)

target_link_libraries(blueduck_report
    PUBLIC
        blueduck_interfaces
        Drogon::Drogon
)

target_include_directories(blueduck_report PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
