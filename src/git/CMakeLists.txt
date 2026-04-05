add_library(blueduck_git STATIC
    GitRepository.cpp
    CredentialProvider.cpp
)

target_link_libraries(blueduck_git
    PUBLIC
        blueduck_interfaces
        LibGit2::LibGit2
        OpenSSL::Crypto
)

target_include_directories(blueduck_git PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
