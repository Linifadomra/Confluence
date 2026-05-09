cmake_minimum_required(VERSION 3.16)
project(Confluence LANGUAGES C VERSION 0.1.0)

set(CMAKE_C_STANDARD 11)

add_library(confluence
    src/dol/dol.c
    src/rarc/rarc.c
    src/yaz0/yaz0.c
)

target_include_directories(confluence PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS confluence
    EXPORT ConfluenceTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT ConfluenceTargets
    FILE ConfluenceTargets.cmake
    NAMESPACE Confluence::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Confluence
)

write_basic_package_version_file(
    ConfluenceConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    cmake/ConfluenceConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ConfluenceConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Confluence
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ConfluenceConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/ConfluenceConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Confluence
)