# Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
#
# SPDX-License-Identifier: MIT

include(AISAddExecutable)
include(AISUseGTest)

set(SHARED_SOURCE_FILES
    "${CMAKE_SOURCE_DIR}/shared/test/magic-word.cpp"
)

set(SHARED_TEST_INCLUDE_PATH
    "${CMAKE_SOURCE_DIR}/shared/test/"
)

set(TEST_SOURCE_FILES
    async.cpp
    batch/batch.cpp
    context.cpp
    buffer.cpp
    driver.cpp
    handle.cpp
    hip.cpp
    io.cpp
    misc.cpp
    mountinfo.cpp
    rocfile.cpp
    stream.cpp
)

set(TEST_SYSINCLS
    ${ROCFILE_SOURCE_PATH}
    ${ROCFILE_INCLUDE_PATH}
    ${ROCFILE_TEST_PATH}
    ${SHARED_TEST_INCLUDE_PATH}
)

ais_add_executable(
    NAME rocfile_tests
    DEPS rocfile_shared
    SRCS ${TEST_SOURCE_FILES} ${SHARED_SOURCE_FILES}
    SYSINCLS ${TEST_SYSINCLS}
)
target_compile_options(rocfile_tests PRIVATE "-UNDEBUG")

# Add gtest
target_link_libraries(rocfile_tests PRIVATE GTest::gmock)
target_link_libraries(rocfile_tests PRIVATE GTest::gtest)
target_link_libraries(rocfile_tests PRIVATE GTest::gtest_main)

ais_gtest_discover_tests(
    rocfile_tests
    PROPERTIES LABELS unit
    TEST_LIST rocfile_unit_tests
)

# The following tests are concurrency testers. They run for a short
# time to sniff out any egregious lock errors. Be aware that they
# may "pass" in CI for a while after buggy code has been committed,
# so test failures may not be due to the commit in which you
# observe the failure.

ais_add_executable(
    NAME state_mt
    DEPS rocfile_shared
    SRCS "state_mt.cpp"
    SYSINCLS ${ROCFILE_SOURCE_PATH} ${ROCFILE_INCLUDE_PATH}
)
add_test(
    NAME state_mt_test
    COMMAND gdb --batch --quiet -ex run -ex "thread apply all bt full" --return-child-result --args $<TARGET_FILE:state_mt>
)
set_tests_properties(state_mt_test PROPERTIES LABELS stress)

ais_add_executable(
    NAME batch_mt
    DEPS rocfile_shared
    SRCS "batch/batch_mt.cpp"
    SYSINCLS ${ROCFILE_SOURCE_PATH} ${ROCFILE_INCLUDE_PATH}
)
add_test(
    NAME batch_mt_test
    COMMAND gdb --batch --quiet -ex run -ex "thread apply all bt full" --return-child-result --args $<TARGET_FILE:batch_mt>
)
set_tests_properties(batch_mt_test PROPERTIES LABELS stress)
