/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "async.h"
#include "backend/asyncop-fallback.h"
#include "hip.h"
#include "hipfile-warnings.h"
#include "mbuffer.h"
#include "mfile.h"
#include "mhip.h"
#include "mstream.h"
#include "msys.h"

#include <cstdint>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gtest/gtest.h>
#include <memory>
#include <sys/stat.h>
#include <sys/types.h>

// Put tests inside the macros to suppress the global constructor
// warnings
HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

using namespace rocFile;
using std::shared_ptr;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Combine;
using ::testing::Eq;
using ::testing::Return;
using ::testing::StrictMock;
using ::testing::Test;
using ::testing::Throw;
using ::testing::Values;
using ::testing::WithParamInterface;

typedef struct stat stat_t;

struct RocFileAsyncOp : public Test {
    RocFileAsyncOp()
        : buffer{std::make_shared<StrictMock<MBuffer>>()}, file{std::make_shared<StrictMock<MFile>>()},
          stream{std::make_shared<StrictMock<MStream>>()}
    {
        EXPECT_CALL(*stream, fixedBufferOffset).Times(AnyNumber()).WillRepeatedly(Return(false));
        EXPECT_CALL(*stream, fixedFileOffset).Times(AnyNumber()).WillRepeatedly(Return(false));
        EXPECT_CALL(*stream, fixedIOSize).Times(AnyNumber()).WillRepeatedly(Return(false));
        EXPECT_CALL(*stream, pageAligned).Times(AnyNumber()).WillRepeatedly(Return(false));

        EXPECT_CALL(*buffer, getBuffer)
            .Times(AnyNumber())
            .WillRepeatedly(Return(reinterpret_cast<hipStream_t>(0xFEFEFEFE)));
    }
    StrictMock<MHip>    mhip;
    StrictMock<MSys>    msys;
    shared_ptr<MBuffer> buffer;
    shared_ptr<MFile>   file;
    shared_ptr<MStream> stream;
};

struct RocFileAsyncMonitor : RocFileAsyncOp {
    AsyncMonitor monitor;
};

static auto
rocfileFlagsPowerSet()
{
    return Combine(Values(0, ROCFILE_STREAM_FIXED_BUF_OFFSET), Values(0, ROCFILE_STREAM_FIXED_FILE_OFFSET),
                   Values(0, ROCFILE_STREAM_FIXED_FILE_SIZE), Values(0, ROCFILE_STREAM_PAGE_ALIGNED_INPUTS));
}

struct RocFileAsyncOpStreamParams
    : public RocFileAsyncOp,
      public WithParamInterface<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>> {
    RocFileAsyncOpStreamParams() : RocFileAsyncOp{}
    {
        auto params            = GetParam();
        bool fixed_buf_offset  = std::get<0>(params) > 0;
        bool fixed_file_offset = std::get<1>(params) > 0;
        bool fixed_file_size   = std::get<2>(params) > 0;
        bool page_aligned      = std::get<3>(params) > 0;
        flags  = std::get<0>(params) | std::get<1>(params) | std::get<2>(params) | std::get<3>(params);
        stream = std::make_shared<StrictMock<MStream>>();
        EXPECT_CALL(*stream, getHipStream)
            .Times(AnyNumber())
            .WillRepeatedly(Return(reinterpret_cast<hipStream_t>(0xDEADBEEF)));
        EXPECT_CALL(*stream, fixedBufferOffset).Times(AnyNumber()).WillRepeatedly(Return(fixed_buf_offset));
        EXPECT_CALL(*stream, fixedFileOffset).Times(AnyNumber()).WillRepeatedly(Return(fixed_file_offset));
        EXPECT_CALL(*stream, fixedIOSize).Times(AnyNumber()).WillRepeatedly(Return(fixed_file_size));
        EXPECT_CALL(*stream, pageAligned).Times(AnyNumber()).WillRepeatedly(Return(page_aligned));
    }
    unsigned flags;
};

TEST_P(RocFileAsyncOpStreamParams, asyncOp_construction_has_correct_variants)
{
    size_t size              = 100;
    off_t  file_offset       = 0;
    off_t  buffer_offset     = 0;
    off_t  bytes_transferred = 0;
    auto   op = std::make_shared<AsyncOp>(IoType::Read, file, buffer, stream, &size, &file_offset,
                                          &buffer_offset, &bytes_transferred);

    // Unfixed flags will be pointers
    if (flags & ROCFILE_STREAM_FIXED_BUF_OFFSET) {
        EXPECT_NO_THROW(std::get<const off_t>(op->buffer_offset));
    }
    else {
        EXPECT_NO_THROW(std::get<off_t *>(op->buffer_offset));
    }
    if (flags & ROCFILE_STREAM_FIXED_FILE_OFFSET) {
        EXPECT_NO_THROW(std::get<const off_t>(op->file_offset));
    }
    else {
        EXPECT_NO_THROW(std::get<off_t *>(op->file_offset));
    }
    if (flags & ROCFILE_STREAM_FIXED_FILE_SIZE) {
        EXPECT_NO_THROW(std::get<size_t>(op->size));
    }
    else {
        EXPECT_NO_THROW(std::get<size_t *>(op->size));
    }
}
INSTANTIATE_TEST_SUITE_P(StreamSuite, RocFileAsyncOpStreamParams, rocfileFlagsPowerSet());

TEST_F(RocFileAsyncOp, AsyncOpFallback_new_uses_pinned_host_memory)
{
    size_t size              = 100;
    off_t  file_offset       = 0;
    off_t  buffer_offset     = 0;
    off_t  bytes_transferred = 0;
    auto   op_data           = std::shared_ptr<void>(new uint8_t[sizeof(AsyncOpFallback)]);
    auto   bounce_buffer     = std::shared_ptr<void>(new uint8_t[size]);
    EXPECT_CALL(mhip, hipHostMalloc).WillOnce(Return(op_data.get())).WillOnce(Return(bounce_buffer.get()));
    EXPECT_CALL(mhip, hipHostGetDevicePointer(Eq(bounce_buffer.get()), _));
    EXPECT_CALL(mhip, hipHostFree(Eq(bounce_buffer.get())));
    EXPECT_CALL(mhip, hipHostFree(Eq(op_data.get())));
    auto op = std::shared_ptr<AsyncOpFallback>(new AsyncOpFallback{
        IoType::Read, file, buffer, stream, &size, &file_offset, &buffer_offset, &bytes_transferred});
}

TEST_F(RocFileAsyncOp, AsyncOpFallback_new_failure_throws_bad_alloc)
{
    size_t size              = 100;
    off_t  file_offset       = 0;
    off_t  buffer_offset     = 0;
    off_t  bytes_transferred = 0;
    auto   op_data           = std::shared_ptr<void>(new uint8_t[sizeof(AsyncOpFallback)]);
    EXPECT_CALL(mhip, hipHostMalloc).WillOnce(Throw(Hip::RuntimeError(hipErrorOutOfMemory)));
    EXPECT_THROW(std::shared_ptr<AsyncOpFallback>(new AsyncOpFallback{IoType::Read, file, buffer, stream,
                                                                      &size, &file_offset, &buffer_offset,
                                                                      &bytes_transferred}),
                 std::bad_alloc);
}

TEST_F(RocFileAsyncOp, AsyncOpFallback_bounce_alloc_failure_throws)
{
    size_t size              = 100;
    off_t  file_offset       = 0;
    off_t  buffer_offset     = 0;
    off_t  bytes_transferred = 0;
    auto   op_data           = std::shared_ptr<void>(new uint8_t[sizeof(AsyncOpFallback)]);
    EXPECT_CALL(mhip, hipHostMalloc)
        .WillOnce(Return(op_data.get()))
        .WillOnce(Throw(Hip::RuntimeError(hipErrorOutOfMemory)));
    EXPECT_CALL(mhip, hipHostFree(Eq(op_data.get())));
    EXPECT_THROW(std::shared_ptr<AsyncOpFallback>(new AsyncOpFallback{IoType::Read, file, buffer, stream,
                                                                      &size, &file_offset, &buffer_offset,
                                                                      &bytes_transferred}),
                 Hip::RuntimeError);
}

TEST_F(RocFileAsyncOp, AsyncOpFallback_bounce_buffer_deleter_failure_calls_syslog)
{
    size_t size              = 100;
    off_t  file_offset       = 0;
    off_t  buffer_offset     = 0;
    off_t  bytes_transferred = 0;
    auto   op_data           = std::shared_ptr<void>(new uint8_t[sizeof(AsyncOpFallback)]);
    auto   bounce_buffer     = std::shared_ptr<void>(new uint8_t[size]);
    EXPECT_CALL(mhip, hipHostMalloc).WillOnce(Return(op_data.get())).WillOnce(Return(bounce_buffer.get()));
    EXPECT_CALL(mhip, hipHostGetDevicePointer(Eq(bounce_buffer.get()), _));
    EXPECT_CALL(mhip, hipHostFree(Eq(bounce_buffer.get())))
        .WillOnce(Throw(Hip::RuntimeError(hipErrorInvalidValue)));
    EXPECT_CALL(mhip, hipHostFree(Eq(op_data.get())));
    EXPECT_CALL(msys, syslog);
    auto op = std::shared_ptr<AsyncOpFallback>(new AsyncOpFallback{
        IoType::Read, file, buffer, stream, &size, &file_offset, &buffer_offset, &bytes_transferred});
}

TEST_F(RocFileAsyncOp, AsyncOpFallback_delete_failure_calls_syslog)
{
    size_t size              = 100;
    off_t  file_offset       = 0;
    off_t  buffer_offset     = 0;
    off_t  bytes_transferred = 0;
    auto   op_data           = std::shared_ptr<void>(new uint8_t[sizeof(AsyncOpFallback)]);
    auto   bounce_buffer     = std::shared_ptr<void>(new uint8_t[size]);
    EXPECT_CALL(mhip, hipHostMalloc).WillOnce(Return(op_data.get())).WillOnce(Return(bounce_buffer.get()));
    EXPECT_CALL(mhip, hipHostGetDevicePointer(Eq(bounce_buffer.get()), _));
    EXPECT_CALL(mhip, hipHostFree(Eq(bounce_buffer.get())));
    EXPECT_CALL(mhip, hipHostFree(Eq(op_data.get())))
        .WillOnce(Throw(Hip::RuntimeError(hipErrorInvalidValue)));
    EXPECT_CALL(msys, syslog);
    auto op = std::shared_ptr<AsyncOpFallback>(new AsyncOpFallback{
        IoType::Read, file, buffer, stream, &size, &file_offset, &buffer_offset, &bytes_transferred});
}

struct RocFileAsyncOpFallbackFunctions : public RocFileAsyncOp {
    RocFileAsyncOpFallbackFunctions() : bounce_buffer{new uint8_t[size]}
    {
        //  make_shared uses placement new, which will not use hipHostMalloc/hipHostFree for
        //  AsyncOpFallback
        EXPECT_CALL(mhip, hipHostMalloc).WillOnce(Return(bounce_buffer.get()));
        EXPECT_CALL(mhip, hipHostGetDevicePointer).WillOnce(Return(bounce_buffer_dev_ptr));
        op = std::make_shared<AsyncOpFallback>(IoType::Read, file, buffer, stream, &size, &file_offset,
                                               &buffer_offset, &bytes_transferred);
    }
    ~RocFileAsyncOpFallbackFunctions() override
    {
        EXPECT_CALL(mhip, hipHostFree(Eq(bounce_buffer.get())));
    }
    void                            *bounce_buffer_dev_ptr = reinterpret_cast<void *>(0xDECDECDE);
    size_t                           size                  = 100;
    off_t                            file_offset           = 0;
    off_t                            buffer_offset         = 0;
    off_t                            bytes_transferred     = 0;
    std::shared_ptr<void>            bounce_buffer;
    std::shared_ptr<AsyncOpFallback> op;
};

TEST_F(RocFileAsyncOpFallbackFunctions, bounceBufferHostPtr_returns_pointer)
{
    ASSERT_EQ(op->bounceBufferHostPtr(), bounce_buffer.get());
}

TEST_F(RocFileAsyncOpFallbackFunctions, devPtr_calls_hipHostGetDevicePointer)
{
    void *addr = reinterpret_cast<void *>(0xABACADBA);
    EXPECT_CALL(mhip, hipHostGetDevicePointer).WillOnce(Return(addr));
    ASSERT_EQ(op->devPtr(), addr);
}

TEST_F(RocFileAsyncMonitor, addOp_and_completeOp_with_valid_params_works)
{
    size_t size              = 100;
    off_t  file_offset       = 0;
    off_t  buffer_offset     = 0;
    off_t  bytes_transferred = 0;
    auto   op = std::make_shared<AsyncOp>(IoType::Read, file, buffer, stream, &size, &file_offset,
                                          &buffer_offset, &bytes_transferred);

    monitor.addOp(op);
    EXPECT_NO_THROW(monitor.completeOp(op.get()));
}

TEST_F(RocFileAsyncMonitor, completeOp_with_invalid_op_throws)
{
    EXPECT_THROW(monitor.completeOp(reinterpret_cast<AsyncOp *>(0xDEADBEEF)), std::invalid_argument);
}

TEST_F(RocFileAsyncMonitor, addOp_without_completeOp_prints_error_on_AsyncMonitor_destruction)
{
    size_t size              = 100;
    off_t  file_offset       = 0;
    off_t  buffer_offset     = 0;
    off_t  bytes_transferred = 0;
    auto   op = std::make_unique<AsyncOp>(IoType::Read, file, buffer, stream, &size, &file_offset,
                                          &buffer_offset, &bytes_transferred);
    monitor.addOp(std::move(op));
    EXPECT_CALL(msys, syslog);
}

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
