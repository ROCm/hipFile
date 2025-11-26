# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.21)

project("hipfile" DESCRIPTION "hipFile library" LANGUAGES CXX HIP)
project("rocfile" DESCRIPTION "rocFile library" LANGUAGES CXX)

# Set the path to the include directories for later use
set(HIPFILE_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(HIPFILE_AMD_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/amd_detail")
set(HIPFILE_NVIDIA_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/nvidia_detail")
set(HIPFILE_TEST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/test")

# Set the path to the include directories for later use
set(ROCFILE_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(ROCFILE_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/amd_detail")
set(ROCFILE_TEST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/test/amd_detail")

# Add the src directory depending on the HIP platform
if(BUILD_NVIDIA_DETAIL)
    add_subdirectory(src/nvidia_detail)
else()
    add_subdirectory(src/amd_detail)
endif()

# Add the testing directories
if(BUILD_TESTING)
    add_subdirectory(test)
endif()
if(BUILD_AMD_DETAIL)
    add_subdirectory(test/amd_detail)
endif()

# Add the aiscp example directory
if(BUILD_AISCP)
    add_subdirectory(examples/aiscp)
endif()
