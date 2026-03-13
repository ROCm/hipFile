/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "io.h"
#include "hipfile-test.h"
#include "hipfile-warnings.h"
#include "mbackend.h"
#include "mbuffer.h"
#include "mfile.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace hipFile;

using ::testing::AnyNumber;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;
using ::testing::Throw;

// Put tests inside the macros to suppress the global constructor
// warnings
HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

struct DummyRetryableBackend : public MRetryableBackend {};

struct DummyFallbackBackend : MBackend {};

struct HipFileRetryableBackend : public HipFileUnopened, ::testing::WithParamInterface<IoType> {
    std::shared_ptr<StrictMock<MBuffer>> mock_buffer;
    std::shared_ptr<StrictMock<MFile>>   mock_file;

    std::shared_ptr<StrictMock<DummyRetryableBackend>> default_backend;
    std::shared_ptr<StrictMock<DummyFallbackBackend>>  fallback_backend;

    IoType  io_type;
    ssize_t successful_io_size = 0x1234;

    void SetUp() override
    {
        mock_buffer = std::make_shared<StrictMock<MBuffer>>();
        mock_file   = std::make_shared<StrictMock<MFile>>();

        default_backend = std::make_shared<StrictMock<DummyRetryableBackend>>();
        // By default, use the default is_retryable() implementation unless overriden.
        EXPECT_CALL(*default_backend, is_retryable)
            .WillRepeatedly(Invoke([&](std::exception_ptr e_ptr, ssize_t nbytes) {
                return default_backend->RetryableBackend::is_retryable(e_ptr, nbytes);
            }));
        EXPECT_CALL(*default_backend, retryable_io).Times(AnyNumber());
        EXPECT_CALL(*default_backend, update_read_stats).Times(AnyNumber());
        EXPECT_CALL(*default_backend, update_write_stats).Times(AnyNumber());

        fallback_backend = std::make_shared<StrictMock<DummyFallbackBackend>>();
        EXPECT_CALL(*fallback_backend, io).Times(AnyNumber());

        io_type = GetParam();
    }

    HipFileRetryableBackend()
    {
    }
};

TEST_P(HipFileRetryableBackend, IOSuccess)
{
    EXPECT_CALL(*default_backend, retryable_io).WillOnce(Return(successful_io_size));

    ssize_t nbytes = default_backend->io(io_type, mock_file, mock_buffer, 0, 0, 0);

    ASSERT_EQ(nbytes, successful_io_size);
}

TEST_P(HipFileRetryableBackend, IOFailureNoFallback)
{
    EXPECT_CALL(*default_backend, retryable_io).WillOnce(Throw(std::runtime_error("IO failure")));

    EXPECT_THROW(default_backend->io(io_type, mock_file, mock_buffer, 0, 0, 0), std::runtime_error);
}

TEST_P(HipFileRetryableBackend, IOFailureWithIneligibleRetry)
{
    // The Backend has registered a fallback, but has determined that the
    // IO error should not be retried.
    default_backend->register_retry_backend(fallback_backend);
    EXPECT_CALL(*default_backend, is_retryable).WillOnce(Return(false));
    EXPECT_CALL(*default_backend, retryable_io).WillOnce(Throw(std::runtime_error("IO failure")));

    EXPECT_THROW(default_backend->io(io_type, mock_file, mock_buffer, 0, 0, 0), std::runtime_error);
}

TEST_P(HipFileRetryableBackend, IOFailureWithGoodFallback)
{
    default_backend->register_retry_backend(fallback_backend);
    EXPECT_CALL(*default_backend, retryable_io).WillOnce(Throw(std::runtime_error("IO failure")));
    EXPECT_CALL(*fallback_backend, io).WillOnce(Return(successful_io_size));

    ssize_t nbytes = default_backend->io(io_type, mock_file, mock_buffer, 0, 0, 0);
    ASSERT_EQ(nbytes, successful_io_size);
}

INSTANTIATE_TEST_SUITE_P(, HipFileRetryableBackend, ::testing::Values(IoType::Read, IoType::Write));

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
