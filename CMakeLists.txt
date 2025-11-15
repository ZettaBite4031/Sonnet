cmake_minimum_required(VERSION 3.20)

project(Sonnet
    VERSION 0.1.0
    DESCRIPTION "Sonnet - A modern allocator-aware JSON library for C++"
    LANGUAGES CXX
)   

# ============================================================
# Target properties
# ============================================================

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
# Options
# ============================================================

option(SONNET_BUILD_SHARED "Build Sonnet as a shared library" ON)
option(SONNET_BUILD_TESTS "Build Sonnet tests" ON)
option(SONNET_INSTALL "Enable Sonnet install rules" ON)
option(SONNET_ENABLE_WARNINGS "Enable strict warning flags" ON)

# ============================================================
# Library target
# ============================================================

# Source / header lists
set(SONNET_PUBLIC_HEADERS
    include/sonnet/config.hpp
    include/sonnet/convert.hpp
    include/sonnet/error.hpp
    include/sonnet/options.hpp
    include/sonnet/value.hpp
    include/sonnet/sonnet.hpp
)

set(SONNET_SOURCES
    src/value.cpp    
    src/error.cpp    
    src/sonnet.cpp    
)

if (SONNET_BUILD_SHARED) 
    add_library(sonnet SHARED ${SONNET_SOURCES} ${SONNET_PUBLIC_HEADERS})
    target_compile_definitions(sonnet
        PRIVATE SONNET_BUILD_SHARED
        PUBLIC SONNET_SHARED
    )
else()
    add_library(sonnet STATIC ${SONNET_SOURCES} ${SONNET_PUBLIC_HEADERS})
    target_compile_definitions(sonnet,
        PUBLIC SONNET_STATIC
    )
endif()

add_library(Sonnet::sonnet ALIAS sonnet)



target_include_directories(sonnet
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

set_target_properties(sonnet PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# On MSVC, export all symbols by default for shared builds
# (not strictly required because we have SONNET_API, but can help)
if(MSVC)
    # Disable min/max macros, etc
    target_compile_definitions(sonnet PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)

    if(SONNET_BUILD_SHARED)
        # Optional: auto export, but SONNET_API should generally be enough
        # set_target_properties(sonnet PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()
endif()

# ============================================================
# Warnings (optional but recommended)
# ============================================================

if(SONNET_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(sonnet PRIVATE
            /W4           # high warning level
            /permissive-  # strict standard conformance
            /EHsc         # C++ exceptions only
        )
    else()
        target_compile_options(sonnet PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wshadow
            -Wold-style-cast
        )
    endif()
endif()


# ============================================================
# Tests
# ============================================================

if(SONNET_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)  # you can put your Catch2/GoogleTest stuff here
endif()

# ============================================================
# Installation and package config
# ============================================================

if(SONNET_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    file(MAKE_DIRECTORY ${CMAKE_INSTALL_INCLUDEDIR}/include/sonnet)
    file(MAKE_DIRECTORY ${CMAKE_INSTALL_LIBDIR}/lib/)
    file(MAKE_DIRECTORY ${CMAKE_INSTALL_LIBDIR}/bin/)

    # ---- Install headers ----
    install(
        DIRECTORY include/sonnet
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    # ---- Install library ----
    install(
        TARGETS sonnet
        EXPORT  SonnetTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    # ---- Export targets for find_package ----
    install(
        EXPORT SonnetTargets
        FILE   SonnetTargets.cmake
        NAMESPACE Sonnet::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Sonnet
    )

    # ---- Configure SonnetConfig.cmake & version file ----
    set(SONNET_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/Sonnet")

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/SonnetConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/SonnetConfig.cmake
        INSTALL_DESTINATION ${SONNET_CONFIG_INSTALL_DIR}
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/SonnetConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/SonnetConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/SonnetConfigVersion.cmake
        DESTINATION ${SONNET_CONFIG_INSTALL_DIR}
    )
endif()