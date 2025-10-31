# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.17)

project("rocfile" DESCRIPTION "rocFile library" LANGUAGES CXX)

# Set the path to the include directory for later use
set(ROCFILE_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(ROCFILE_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(ROCFILE_TEST_PATH "${CMAKE_CURRENT_SOURCE_DIR}/test")

add_subdirectory(src)

# Add the testing directory
if(BUILD_TESTING)
    add_subdirectory(test)
endif()
