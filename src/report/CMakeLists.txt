add_library(blueduck_report STATIC
    ReportEngine.cpp
)

target_link_libraries(blueduck_report
    PUBLIC
        blueduck_interfaces
        drogon
)

target_include_directories(blueduck_report PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
