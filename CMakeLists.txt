cmake_minimum_required(VERSION 3.20)

project(otpkit
    VERSION 0.1.0
    DESCRIPTION "Portable C++17 library for HOTP and TOTP (RFC 4226 / RFC 6238)"
    LANGUAGES CXX
)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install")
endif()

# ---------------------------------------------------------------------------
# Options
#
# By default, only the core library is built. Tests and examples are
# opt-in so that consuming projects (and vcpkg ports) get a minimal
# build with no extra dependencies.
#
# Usage:
#   -DVCPKG_MANIFEST_FEATURES="tests"   → installs Catch2
#   -DOTPKIT_BUILD_TESTS=ON             → builds tests
#   -DOTPKIT_BUILD_EXAMPLES=ON          → builds examples
# ---------------------------------------------------------------------------
option(OTPKIT_BUILD_TESTS    "Build unit tests"  OFF)
option(OTPKIT_BUILD_EXAMPLES "Build examples"    OFF)

# If the user enabled the vcpkg feature "tests", turn tests on
# automatically (unless they explicitly set OTPKIT_BUILD_TESTS=OFF).
if(DEFINED VCPKG_MANIFEST_FEATURES AND NOT VCPKG_MANIFEST_FEATURES STREQUAL "")
    message(STATUS "otpkit: VCPKG_MANIFEST_FEATURES = ${VCPKG_MANIFEST_FEATURES}")
    if("tests" IN_LIST VCPKG_MANIFEST_FEATURES)
        set(OTPKIT_BUILD_TESTS ON)
    endif()
endif()

message(STATUS "otpkit: TESTS=${OTPKIT_BUILD_TESTS} EXAMPLES=${OTPKIT_BUILD_EXAMPLES}")

# ---------------------------------------------------------------------------
# Core library (no external dependencies)
# ---------------------------------------------------------------------------
add_library(otpkit_core
    src/core/sha1.cpp
    src/core/sha256.cpp
    src/core/sha512.cpp
    src/core/hmac.cpp
    src/core/base32.cpp
    src/hotp.cpp
    src/totp.cpp
)
add_library(otpkit::core ALIAS otpkit_core)

target_include_directories(otpkit_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(WIN32)
    target_sources(otpkit_core PRIVATE src/platform/random_win.cpp)
    target_link_libraries(otpkit_core PRIVATE bcrypt)
elseif(UNIX)
    target_sources(otpkit_core PRIVATE src/platform/random_posix.cpp)
endif()

# ---------------------------------------------------------------------------
# Tests (optional)
# ---------------------------------------------------------------------------
if(OTPKIT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# Examples (optional)
# ---------------------------------------------------------------------------
if(OTPKIT_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ---------------------------------------------------------------------------
# Install & export
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS otpkit_core
    EXPORT otpkitTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/otpkit
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT otpkitTargets
    FILE otpkitTargets.cmake
    NAMESPACE otpkit::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/otpkit
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/otpkitConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/otpkitConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/otpkit
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/otpkitConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/otpkitConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/otpkitConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/otpkit
)
