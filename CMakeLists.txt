cmake_minimum_required(VERSION 3.20)
project(LiquidRocketPiping VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt 6 (relax minimum to 6.2 for wider compatibility; 6.8 recommended)
find_package(Qt6 6.2 QUIET COMPONENTS Widgets Svg)
if(NOT Qt6_FOUND)
    message(WARNING "Qt6 not found. Build will be configured but cannot compile GUI code.")
    message(WARNING "Install Qt 6.x and set CMAKE_PREFIX_PATH to the Qt install directory.")
endif()

# Eigen and ExprTk — optional math libraries
# Install via vcpkg: vcpkg install eigen3 exprtk
# Configure with: cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=E:/vcpkg/scripts/buildsystems/vcpkg.cmake
find_package(Eigen3 QUIET)
if(Eigen3_FOUND)
    message(STATUS "Eigen3 found — enabling Eigen-based computations")
    set(USE_EIGEN ON CACHE BOOL "Eigen3 available")
else()
    message(STATUS "Eigen3 not found — math stubs will be used")
endif()

find_path(EXPRTK_INCLUDE_DIR "exprtk.hpp")
if(EXPRTK_INCLUDE_DIR)
    message(STATUS "ExprTk found at ${EXPRTK_INCLUDE_DIR} — enabling symbolic math")
    set(USE_EXPRTK ON CACHE BOOL "ExprTk available")
    add_library(exprtk INTERFACE)
    target_include_directories(exprtk INTERFACE "${EXPRTK_INCLUDE_DIR}")
else()
    message(STATUS "ExprTk not found — math stubs will be used")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)

add_subdirectory(src)

# Tests (optional — require Catch2)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    find_package(Catch2 3 QUIET)
    if(Catch2_FOUND)
        enable_testing()
        add_subdirectory(tests)
    else()
        message(STATUS "Catch2 not found — tests disabled. Install via vcpkg or system package manager.")
    endif()
endif()
