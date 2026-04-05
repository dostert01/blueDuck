enable_testing()
find_package(GTest REQUIRED)

# ---------------------------------------------------------------
# Unit tests (no DB required)
# ---------------------------------------------------------------
add_executable(blueduck_unit_tests
    unit/test_version_compare.cpp
    unit/test_cmake_analyzer.cpp
    unit/test_nvd_parser.cpp
    unit/test_python_analyzer.cpp
)

target_include_directories(blueduck_unit_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/plugins
)

target_link_libraries(blueduck_unit_tests
    PRIVATE
        GTest::GTest
        GTest::Main
        blueduck_report      # ReportEngine (version compare)
        blueduck_plugin_cmake_cpp
        blueduck_plugin_python
        blueduck_plugin_nvd
        nlohmann_json::nlohmann_json
        PostgreSQL::PostgreSQL
)

add_test(NAME UnitTests COMMAND blueduck_unit_tests)

# ---------------------------------------------------------------
# Integration tests (require BD_TEST_DB_URL)
# ---------------------------------------------------------------
add_executable(blueduck_integration_tests
    integration/test_db_migrator.cpp
)

target_include_directories(blueduck_integration_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(blueduck_integration_tests
    PRIVATE
        GTest::GTest
        GTest::Main
        blueduck_db
        Drogon::Drogon
)

add_test(NAME IntegrationTests COMMAND blueduck_integration_tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
