## add_blueduck_plugin(<target> TYPE <type> SOURCES ... [INCLUDE_DIRS ...] [LIBRARIES ...])
##
## Creates a SHARED library (plugin) with the standard blueDuck plugin setup:
##   - Links blueduck_interfaces
##   - Adds interfaces/ and plugins/ to include directories
##   - Strips the "lib" prefix so dlopen can find it by plugin name
##   - TYPE is "cve_source" or "dependency_analyzer"; determines the output
##     subdirectory under build/plugins/<type>/
function(add_blueduck_plugin target_name)
    cmake_parse_arguments(PLUGIN "" "TYPE" "SOURCES;INCLUDE_DIRS;LIBRARIES" ${ARGN})

    add_library(${target_name} SHARED ${PLUGIN_SOURCES})

    target_include_directories(${target_name}
        PRIVATE
            ${CMAKE_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/plugins
            ${PLUGIN_INCLUDE_DIRS}
    )

    target_link_libraries(${target_name}
        PRIVATE
            ${PLUGIN_LIBRARIES}
    )

    if(PLUGIN_TYPE)
        set(_output_dir ${CMAKE_BINARY_DIR}/plugins/${PLUGIN_TYPE})
    else()
        set(_output_dir ${CMAKE_BINARY_DIR}/plugins)
    endif()

    set_target_properties(${target_name} PROPERTIES
        PREFIX ""
        SUFFIX ".so"
        LIBRARY_OUTPUT_DIRECTORY ${_output_dir}
    )
endfunction()
