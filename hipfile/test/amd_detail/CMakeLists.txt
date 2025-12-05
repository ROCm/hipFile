# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

include(AISAddExecutable)
include(AISUseGTest)

set(SHARED_SOURCE_FILES
    "${CMAKE_SOURCE_DIR}/hipfile/test/common/magic-word.cpp"
)

set(TEST_SOURCE_FILES
    async.cpp
    batch/batch.cpp
    configuration.cpp
    context.cpp
    buffer.cpp
    driver.cpp
    environment.cpp
    handle.cpp
    hip.cpp
    hipfile-api.cpp
    fallback.cpp
    fastpath.cpp
    mountinfo.cpp
    stream.cpp
)

set(TEST_SYSINCLS
    ${HIPFILE_AMD_SOURCE_PATH}
    ${HIPFILE_INCLUDE_PATH}
    ${HIPFILE_TEST_INCLUDE_PATH}
    ${HIPFILE_AMD_TEST_PATH}
)

ais_add_executable(
    NAME internal_tests
    DEPS hipfile_shared
    SRCS ${TEST_SOURCE_FILES} ${SHARED_SOURCE_FILES}
    SYSINCLS ${TEST_SYSINCLS}
)
target_compile_options(internal_tests PRIVATE "-UNDEBUG")

# Add gtest
target_link_libraries(internal_tests PRIVATE GTest::gmock)
target_link_libraries(internal_tests PRIVATE GTest::gtest)
target_link_libraries(internal_tests PRIVATE GTest::gtest_main)

ais_gtest_discover_tests(
    internal_tests
    PROPERTIES "LABELS;unit;LABELS;internal"
    TEST_LIST internal_unit_tests
)

# The following tests are concurrency testers. They run for a short
# time to sniff out any egregious lock errors. Be aware that they
# may "pass" in CI for a while after buggy code has been committed,
# so test failures may not be due to the commit in which you
# observe the failure.

ais_add_executable(
    NAME state_mt
    DEPS hipfile_shared
    SRCS "state_mt.cpp"
    SYSINCLS ${HIPFILE_AMD_SOURCE_PATH} ${HIPFILE_INCLUDE_PATH}
)
add_test(
    NAME state_mt_test
    COMMAND gdb --batch --quiet -ex run -ex "thread apply all bt full" --return-child-result --args $<TARGET_FILE:state_mt>
)
set_tests_properties(state_mt_test PROPERTIES LABELS "stress;internal")

ais_add_executable(
    NAME batch_mt
    DEPS hipfile_shared
    SRCS "batch/batch_mt.cpp"
    SYSINCLS ${HIPFILE_AMD_SOURCE_PATH} ${HIPFILE_INCLUDE_PATH}
)
add_test(
    NAME batch_mt_test
    COMMAND gdb --batch --quiet -ex run -ex "thread apply all bt full" --return-child-result --args $<TARGET_FILE:batch_mt>
)
set_tests_properties(batch_mt_test PROPERTIES LABELS "stress;internal")
