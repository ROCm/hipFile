# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

include(FetchContent)

FetchContent_Declare(
    rocm-cmake
    URL https://github.com/ROCm/rocm-cmake/archive/refs/tags/rocm-6.4.3.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP true
    SOURCE_SUBDIR "DISABLE ADDING TO BUILD"
    SYSTEM
    FIND_PACKAGE_ARGS NAMES ROCmCMakeBuildTools
)
FetchContent_MakeAvailable(rocm-cmake)

if(rocm-cmake_SOURCE_DIR)
    message(STATUS "Using fetched rocm-cmake.")
    find_package(ROCmCMakeBuildTools CONFIG REQUIRED NO_DEFAULT_PATH PATHS "${rocm-cmake_SOURCE_DIR}")
else()
    message(STATUS "Using system rocm-cmake.")
endif()
