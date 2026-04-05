add_library(blueduck_db STATIC
    Database.cpp
    Migrator.cpp
)

target_link_libraries(blueduck_db
    PUBLIC
        Drogon::Drogon
        blueduck_interfaces
        PostgreSQL::PostgreSQL
)

target_include_directories(blueduck_db PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
