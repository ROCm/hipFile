/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/*
 * These tests are integration-level tests against th rocFile API.
 * They should test that the various rocFile modules are compatible with
 * one another. Running IO is not intended to be guaranteed to run here.
 *
 * All tests in this file are expected to pass on a CPU-only node.
 */

#include "batch/batch.h"
#include "context.h"
#include "hip.h"
#include "hipfile-warnings.h"
#include "io.h"
#include "mbackend.h"
#include "mbatch.h"
#include "mhip.h"
#include "mmountinfo.h"
#include "mstate.h"
#include "msys.h"
#include "rocfile-private.h"
#include "rocfile-test.h"
#include "rocfile.h"
#include "state.h"
#include "sys.h"

#include <array>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hip/hip_runtime_api.h>
#include <memory>
#include <stdexcept>
#include <sys/types.h>
#include <vector>

namespace rocFile {
struct Backend;
}

using namespace rocFile;
using namespace std;
using namespace testing;

// Put tests inside the macros to suppress the global constructor
// warnings
HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

struct RocFileUnit : public RocFileUnopened {
    StrictMock<MDriverState> mock_state;
    void                     SetUp() override
    {
    }
};

TEST_F(RocFileUnit, TestRocFileBatchIOSetupSuccess)
{
    rocFileBatchHandle_t b_handle          = nullptr;
    rocFileBatchHandle_t expected_b_handle = reinterpret_cast<rocFileBatchHandle_t>(0x12345678);

    EXPECT_CALL(mock_state, createBatchContext).WillOnce(Return(expected_b_handle));

    auto result = rocFileBatchIOSetUp(&b_handle, 1);
    EXPECT_EQ(result, ROCFILE_SUCCESS);
    EXPECT_EQ(b_handle, expected_b_handle);
}

TEST_F(RocFileUnit, TestRocFileBatchIOSetupBadArgument)
{
    rocFileBatchHandle_t b_handle = nullptr;

    EXPECT_CALL(mock_state, createBatchContext).WillOnce(Throw(std::invalid_argument("")));

    auto result = rocFileBatchIOSetUp(&b_handle, 0);
    EXPECT_EQ(result, ROCFILE_INVALID_VALUE);
    EXPECT_EQ(b_handle, nullptr);
}

TEST_F(RocFileUnit, TestRocFileBatchIOSetupNullptrHandle)
{
    auto result = rocFileBatchIOSetUp(nullptr, 1);
    ASSERT_EQ(result, ROCFILE_INVALID_VALUE);
}

TEST_F(RocFileUnit, TestRocFileBatchIOSubmitSuccess)
{
    rocFileBatchHandle_t           b_handle = reinterpret_cast<rocFileBatchHandle_t>(0x12345678);
    rocFileIOParams_t              io_param;
    std::shared_ptr<MBatchContext> mock_b_context = std::make_shared<MBatchContext>();

    EXPECT_CALL(mock_state, getBatchContext).WillOnce(Return(mock_b_context));
    EXPECT_CALL(*mock_b_context, submit_operations);

    auto result = rocFileBatchIOSubmit(b_handle, 1, &io_param, 0);
    ASSERT_EQ(result, ROCFILE_SUCCESS);
}

TEST_F(RocFileUnit, TestRocFileBatchIOSubmitBadHandle)
{
    rocFileBatchHandle_t           b_handle = nullptr;
    rocFileIOParams_t              io_param;
    std::shared_ptr<MBatchContext> mock_b_context = std::make_shared<MBatchContext>();

    EXPECT_CALL(mock_state, getBatchContext).WillOnce(Throw(InvalidBatchHandle()));
    EXPECT_CALL(*mock_b_context, submit_operations).Times(0);

    auto           result          = rocFileBatchIOSubmit(b_handle, 1, &io_param, 0);
    rocFileError_t expected_result = {rocFileInvalidValue, hipSuccess};
    ASSERT_EQ(result, expected_result);
}

TEST_F(RocFileUnit, TestRocFileBatchIOSubmitBadArgument)
{
    rocFileBatchHandle_t           b_handle = reinterpret_cast<rocFileBatchHandle_t>(0x12345678);
    rocFileIOParams_t              io_param;
    std::shared_ptr<MBatchContext> mock_b_context = std::make_shared<MBatchContext>();

    EXPECT_CALL(mock_state, getBatchContext).WillOnce(Return(mock_b_context));
    EXPECT_CALL(*mock_b_context, submit_operations).WillOnce(Throw(std::invalid_argument("")));

    auto result = rocFileBatchIOSubmit(b_handle, 1, &io_param, 0);
    ASSERT_EQ(result, ROCFILE_INVALID_VALUE);
}

/// @brief Test rocFileIO function
struct RocFileIoParam : public TestWithParam<IoType> {
    rocFileHandle_t                  file_handle{};
    void                            *bufptr{reinterpret_cast<void *>(0xDEC0DE)};
    size_t                           buflen{4096};
    const void                      *unreg_bufptr{reinterpret_cast<void *>(0xFACEFEED)};
    shared_ptr<StrictMock<MBackend>> mbackend{make_shared<StrictMock<MBackend>>()};
    vector<shared_ptr<Backend>>      mbackends{mbackend};

    RocFileIoParam()
    {
        {
            StrictMock<MSys>            msys;
            StrictMock<MLibMountHelper> mlmh;
            expect_file_registration(msys, mlmh);
            file_handle = Context<DriverState>::get()->registerFile(0x0BADCAFE);
        }

        {
            StrictMock<MHip> mhip;
            expect_buffer_registration(mhip, hipMemoryTypeDevice);
            Context<DriverState>::get()->registerBuffer(bufptr, buflen, 0);
        }
    }
};

TEST_P(RocFileIoParam, RocFileIoHandlesHipPointerGetAttributesError)
{
    StrictMock<MHip> mhip;
    EXPECT_CALL(mhip, hipPointerGetAttributes).WillOnce(testing::Throw(Hip::RuntimeError(hipErrorUnknown)));
    ASSERT_EQ(rocFileIo(GetParam(), file_handle, unreg_bufptr, 0, 0, 0, mbackends), -hipErrorUnknown);
}

TEST_P(RocFileIoParam, RocFileIoHandlesUnsupportedHipMemoryType)
{
    for (const auto memoryType : UnsupportedHipMemoryTypes) {
        StrictMock<MHip>      mhip;
        hipPointerAttribute_t attrs{};
        attrs.type = memoryType;
        EXPECT_CALL(mhip, hipPointerGetAttributes).WillOnce(testing::Return(attrs));
        ASSERT_EQ(rocFileIo(GetParam(), file_handle, unreg_bufptr, 0, 0, 0, mbackends),
                  -static_cast<ssize_t>(rocFileHipMemoryTypeInvalid));
    }
}

TEST_P(RocFileIoParam, RocFileIoHandlesInvalidRegisteredBufferLength)
{
    StrictMock<MHip> mhip;
    ASSERT_EQ(rocFileIo(GetParam(), file_handle, bufptr, buflen + 1, 0, 0, mbackends),
              -static_cast<ssize_t>(rocFileInvalidValue));
}

TEST_P(RocFileIoParam, RocFileIoHandlesInvalidFileHandle)
{
    auto invalid_handle{reinterpret_cast<rocFileHandle_t>(0xdeadbeef)};
    ASSERT_EQ(rocFileIo(GetParam(), invalid_handle, bufptr, 0, 0, 0, mbackends), -rocFileHandleNotRegistered);
}

TEST_P(RocFileIoParam, RocFileIoHandlesSysRuntimeError)
{
    EXPECT_CALL(*mbackend, score).WillOnce(Return(1));
    EXPECT_CALL(*mbackend, io).WillOnce(Throw(Sys::RuntimeError(EBADFD)));
    errno = 0;
    ASSERT_EQ(rocFileIo(GetParam(), file_handle, bufptr, buflen, 0, 0, mbackends), -1);
    ASSERT_EQ(errno, EBADFD);
}

INSTANTIATE_TEST_SUITE_P(RocFileIo, RocFileIoParam, Values(IoType::Read, IoType::Write));

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
