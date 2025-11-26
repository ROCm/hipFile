# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

include(AISAddExecutable)
include(AISUseGTest)
find_package(Boost COMPONENTS program_options REQUIRED)

set(SHARED_SOURCE_FILES
    "${CMAKE_SOURCE_DIR}/shared/test/magic-word.cpp"
)

set(SHARED_TEST_INCLUDE_PATH
    "${CMAKE_SOURCE_DIR}/shared/test/"
)

# These tests do not require a HIP capable GPU.
# They should pass on a CPU-only system.
set(UNIT_TEST_SOURCE_FILES
    main.cpp
)

# These tests call into the GPU Driver.
# They will not pass if a HIP capable GPU is not present
# and configured on the system.
set(SYSTEM_TEST_SOURCE_FILES
    system/buffer.cpp
    system/config.cpp
    system/driver.cpp
    system/main.cpp
    system/version.cpp
)

set(TEST_SYSINCLS
    ${HIPFILE_INCLUDE_PATH}
    ${HIPFILE_TEST_PATH}
    ${SHARED_TEST_INCLUDE_PATH}
)

if(BUILD_NVIDIA_DETAIL)
    list(APPEND UNIT_TEST_SOURCE_FILES hipfile-cufile.cpp)
    list(APPEND TEST_SYSINCLS ${HIPFILE_NVIDIA_SOURCE_PATH})
else()
    list(APPEND UNIT_TEST_SOURCE_FILES hipfile-rocfile.cpp)
    list(APPEND TEST_SYSINCLS ${HIPFILE_AMD_SOURCE_PATH})
endif()

ais_add_executable(
    NAME hipfile_tests
    DEPS hipfile_shared
    SRCS ${UNIT_TEST_SOURCE_FILES} ${SHARED_SOURCE_FILES}
    SYSINCLS ${TEST_SYSINCLS}
)
target_compile_options(hipfile_tests
    PRIVATE -Wno-unused-function
)
target_link_libraries(hipfile_tests PRIVATE GTest::gtest)
target_link_libraries(hipfile_tests PRIVATE GTest::gtest_main)

ais_gtest_discover_tests(
    hipfile_tests
    PROPERTIES "LABELS;unit;LABELS;hipfile"
    TEST_LIST hipfile_unit_tests
)

ais_add_executable(
    NAME hipfile_system_tests
    DEPS hipfile_shared
    SRCS ${SYSTEM_TEST_SOURCE_FILES} ${SHARED_SOURCE_FILES}
    SYSINCLS ${TEST_SYSINCLS}
)
target_link_libraries(hipfile_system_tests PRIVATE GTest::gtest)
target_link_libraries(hipfile_system_tests PRIVATE Boost::program_options)

gtest_discover_tests(
    hipfile_system_tests
    EXTRA_ARGS --ais-capable-dir ${AIS_CAPABLE_DIR}
    WORKING_DIRECTORY ${AIS_CAPABLE_DIR}
    PROPERTIES "LABELS;system;LABELS;hipfile"
)

# Temporary Test Files
# Can be deleted once proper testing added
# ----------------------------------------
ais_add_executable(
    NAME test_hipfile_basic
    SRCS legacy/test-hipfile.cpp
    DEPS hipfile_shared
)
ais_add_executable(
    NAME test_hipfile_batch
    SRCS legacy/test-hipfile-batch.cpp
    DEPS hipfile_shared
)
# ----------------------------------------

# These need setup and command-line options (TBD)
#add_test(NAME test_hipfile_basic COMMAND test_hipfile_basic)
#add_test(NAME test_hipfile_batch COMMAND test_hipfile_batch)
# ----------------------------------------
