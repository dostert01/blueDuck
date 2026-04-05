add_library(blueduck_interfaces INTERFACE)

target_include_directories(blueduck_interfaces INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
