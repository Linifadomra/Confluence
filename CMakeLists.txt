cmake_minimum_required(VERSION 3.16)
project(Confluence LANGUAGES C VERSION 0.1.0)

set(CMAKE_C_STANDARD 11)

option(CONFLUENCE_DOL_YAML "DOL/YAML support" ON)

add_library(confluence
    src/dol/dol.c
    src/rarc/rarc.c
    src/yaz0/yaz0.c
)

target_include_directories(confluence PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(CONFLUENCE_DOL_YAML)
    target_sources(confluence PRIVATE src/yaml/yaml.c)
    target_compile_definitions(confluence PUBLIC CONFLUENCE_DOL_YAML=1)

    find_package(yaml QUIET)
    if(NOT yaml_FOUND)
        find_package(PkgConfig QUIET)

        if(PkgConfig_FOUND)
            pkg_check_modules(YAML IMPORTED_TARGET yaml-0.1)
        endif()

        if(NOT YAML_FOUND)
            find_library(YAML_LIBRARY yaml)
            find_path   (YAML_INCLUDE yaml.h)
            if(YAML_LIBRARY AND YAML_INCLUDE)
                if(NOT TARGET yaml)
                    add_library(yaml UNKNOWN IMPORTED)
                    set_target_properties(yaml PROPERTIES
                        IMPORTED_LOCATION             "${YAML_LIBRARY}"
                        INTERFACE_INCLUDE_DIRECTORIES "${YAML_INCLUDE}")
                endif()
            else()
                message(FATAL_ERROR
                    "libyaml not found. Install it with your package manager "
                    "or point YAML_LIBRARY and YAML_INCLUDE at your custom installation.")
            endif()
        endif()
    endif()

    if(TARGET PkgConfig::YAML)
        target_link_libraries(confluence PUBLIC PkgConfig::YAML)
    elseif(TARGET yaml)
        target_link_libraries(confluence PUBLIC yaml)
    else()
        message(FATAL_ERROR "Could not resolve a libyaml target.")
    endif()
endif()

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