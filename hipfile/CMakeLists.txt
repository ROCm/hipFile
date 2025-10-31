# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.21)

project("hipfile" DESCRIPTION "hipFile library" LANGUAGES CXX HIP)

# Set the path to the include directory for later use
set(HIPFILE_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(HIPFILE_AMD_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/amd_detail")
set(HIPFILE_NVIDIA_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/nvidia_detail")
set(HIPFILE_TEST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/test")

# Add the src directory depending on the HIP platform
if(CMAKE_HIP_PLATFORM STREQUAL "nvidia")
    add_subdirectory(src/nvidia_detail)
else()
    add_subdirectory(src/amd_detail)
endif()

# Add the testing directory
if(BUILD_TESTING)
    add_subdirectory(test)
endif()
