add_subdirectory(core)
add_subdirectory(db)
add_subdirectory(git)
add_subdirectory(report)
add_subdirectory(api)

add_executable(blueduck main.cpp)

target_link_libraries(blueduck
    PRIVATE
        blueduck_core
        blueduck_db
        blueduck_git
        blueduck_report
        -Wl,--whole-archive
        blueduck_api
        -Wl,--no-whole-archive
        blueduck_interfaces
        Drogon::Drogon
        ${CMAKE_DL_LIBS}
)
