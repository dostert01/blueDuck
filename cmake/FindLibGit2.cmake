find_path(LIBGIT2_INCLUDE_DIR git2.h)
find_library(LIBGIT2_LIBRARY NAMES git2)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibGit2
    REQUIRED_VARS LIBGIT2_LIBRARY LIBGIT2_INCLUDE_DIR)

if(LibGit2_FOUND AND NOT TARGET LibGit2::LibGit2)
    add_library(LibGit2::LibGit2 UNKNOWN IMPORTED)
    set_target_properties(LibGit2::LibGit2 PROPERTIES
        IMPORTED_LOCATION "${LIBGIT2_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBGIT2_INCLUDE_DIR}")
endif()
