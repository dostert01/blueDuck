add_blueduck_plugin(blueduck_plugin_nvd
    TYPE cve_source
    SOURCES
        NvdSource.cpp
        NvdParser.cpp
        ../../../plugins/common/CveHttpClient.cpp
        ../../../plugins/common/CveDbWriter.cpp
    INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/plugins
    LIBRARIES
        CURL::libcurl
        nlohmann_json::nlohmann_json
        PostgreSQL::PostgreSQL
)
