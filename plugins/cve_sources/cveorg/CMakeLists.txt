add_blueduck_plugin(blueduck_plugin_cveorg
    TYPE cve_source
    SOURCES
        CveOrgSource.cpp
        CveOrgParser.cpp
        ../../../plugins/common/CveHttpClient.cpp
        ../../../plugins/common/CveDbWriter.cpp
    INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/plugins
    LIBRARIES
        CURL::libcurl
        nlohmann_json::nlohmann_json
        PostgreSQL::PostgreSQL
        libzip::zip
)
