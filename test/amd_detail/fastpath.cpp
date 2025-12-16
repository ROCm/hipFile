/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "backend/fastpath.h"
#include "hip.h"
#include "hipfile.h"
#include "hipfile-test.h"
#include "hipfile-warnings.h"
#include "io.h"
#include "mbuffer.h"
#include "mfile.h"
#include "mhip.h"

#include <array>
#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hip/hip_runtime_api.h>
#include <linux/stat.h>
#include <memory>
#include <stdexcept>
#include <sys/types.h>
#include <system_error>

using namespace hipFile;
using namespace testing;
using namespace std;

HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

static int       SCORE_ACCEPT{100};
static const int SCORE_REJECT{-1};

#if defined(STATX_DIOALIGN)
static uint32_t DEFAULT_MEM_ALIGN{4};
static uint32_t DEFAULT_OFFSET_ALIGN{512};
#else
static uint32_t DEFAULT_MEM_ALIGN{4096};
static uint32_t DEFAULT_OFFSET_ALIGN{4096};
#endif

namespace hipFile {
inline bool
operator==(const hipAmdFileHandle_t &lhs, const hipAmdFileHandle_t &rhs)
{
    return lhs.fd == rhs.fd;
}
}

// Provide default values for variables used in fastpath tests
struct FastpathTestBase {
    const size_t        DEFAULT_IO_SIZE{1024 * 1024};
    void *const         DEFAULT_BUFFER_ADDR{reinterpret_cast<void *>(0x600DB10C)};
    const off_t         DEFAULT_BUFFER_OFFSET{512};
    const size_t        DEFAULT_BUFFER_LENGTH{DEFAULT_IO_SIZE + static_cast<size_t>(DEFAULT_BUFFER_OFFSET)};
    const hipMemoryType DEFAULT_BUFFER_TYPE{hipMemoryTypeDevice};
    const int           DEFAULT_FILE_DESCRIPTOR{7};
    const int           DEFAULT_FILE_FLAGS{O_DIRECT};
    const off_t         DEFAULT_FILE_OFFSET{8192};
    const struct statx  DEFAULT_STATX {
        []() {
            struct statx stx {};
#if defined(STATX_DIOALIGN)
            stx.stx_mask             = STATX_DIOALIGN;
            stx.stx_dio_mem_align    = DEFAULT_MEM_ALIGN;
            stx.stx_dio_offset_align = DEFAULT_OFFSET_ALIGN;
#endif
            return stx;
        }()
    };

    // Buffer and file mocks used to setup expectations
    shared_ptr<StrictMock<MFile>>   mfile{make_shared<StrictMock<MFile>>()};
    shared_ptr<StrictMock<MBuffer>> mbuffer{make_shared<StrictMock<MBuffer>>()};
};

struct FastpathTest : public FastpathTestBase, public Test {};

TEST_F(FastpathTest, DefaultExpecations)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET, DEFAULT_BUFFER_OFFSET),
              SCORE_ACCEPT);
}

TEST_F(FastpathTest, BufferedFile)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS & ~O_DIRECT));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET, DEFAULT_BUFFER_OFFSET),
              SCORE_REJECT);
}

TEST_F(FastpathTest, ScoreRejectsNegativeAlignedFileOffset)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, -static_cast<hoff_t>(DEFAULT_OFFSET_ALIGN),
                               DEFAULT_BUFFER_OFFSET),
              SCORE_REJECT);
}

TEST_F(FastpathTest, ScoreRejectsNegativeAlignedBufferOffset)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_BUFFER_OFFSET,
                               -static_cast<hoff_t>(DEFAULT_MEM_ALIGN)),
              SCORE_REJECT);
}

struct FastpathSupportedHipMemoryParam : public FastpathTestBase, public TestWithParam<hipMemoryType> {};

TEST_P(FastpathSupportedHipMemoryParam, Score)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(GetParam()));
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET, DEFAULT_BUFFER_OFFSET),
              SCORE_ACCEPT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathSupportedHipMemoryParam, ValuesIn(SupportedHipMemoryTypes));

struct FastpathUnsupportedHipMemoryParam : public FastpathTestBase, public TestWithParam<hipMemoryType> {};

TEST_P(FastpathUnsupportedHipMemoryParam, Score)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(GetParam()));
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET, DEFAULT_BUFFER_OFFSET),
              SCORE_REJECT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathUnsupportedHipMemoryParam,
                         ValuesIn(UnsupportedHipMemoryTypes));

struct FastpathAlignedIoSizesParam : public FastpathTestBase, public TestWithParam<size_t> {};

TEST_P(FastpathAlignedIoSizesParam, Score)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, GetParam(), DEFAULT_FILE_OFFSET, DEFAULT_BUFFER_OFFSET),
              SCORE_ACCEPT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathAlignedIoSizesParam,
                         Values(-DEFAULT_OFFSET_ALIGN, 0, DEFAULT_OFFSET_ALIGN));

struct FastpathUnalignedIoSizesParam : public FastpathTestBase, public TestWithParam<size_t> {};

TEST_P(FastpathUnalignedIoSizesParam, Score)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, GetParam(), DEFAULT_FILE_OFFSET, DEFAULT_BUFFER_OFFSET),
              SCORE_REJECT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathUnalignedIoSizesParam,
                         Values(-DEFAULT_OFFSET_ALIGN + 1, 1, DEFAULT_OFFSET_ALIGN - 1));

struct FastpathAlignedFileOffsetsParam : public FastpathTestBase, public TestWithParam<hoff_t> {};

TEST_P(FastpathAlignedFileOffsetsParam, Score)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, GetParam(), DEFAULT_BUFFER_OFFSET),
              SCORE_ACCEPT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathAlignedFileOffsetsParam,
                         Values(-DEFAULT_OFFSET_ALIGN, 0, DEFAULT_OFFSET_ALIGN));

struct FastpathUnalignedFileOffsetsParam : public FastpathTestBase, public TestWithParam<hoff_t> {};

TEST_P(FastpathUnalignedFileOffsetsParam, Score)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, GetParam(), DEFAULT_BUFFER_OFFSET),
              SCORE_REJECT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathUnalignedFileOffsetsParam,
                         Values(-DEFAULT_OFFSET_ALIGN + 1, 1, DEFAULT_OFFSET_ALIGN - 1));

struct FastpathAlignedBufferOffsetsParam : public FastpathTestBase, public TestWithParam<hoff_t> {};

TEST_P(FastpathAlignedBufferOffsetsParam, Score)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_BUFFER_OFFSET, GetParam()),
              SCORE_ACCEPT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathAlignedBufferOffsetsParam,
                         Values(-DEFAULT_MEM_ALIGN, 0, DEFAULT_MEM_ALIGN));

/// @brief Tests negative and unaligned buffer offsets
struct FastpathUnalignedBufferOffsetsParam : public FastpathTestBase, public TestWithParam<hoff_t> {};

TEST_P(FastpathUnalignedBufferOffsetsParam, Score)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET, GetParam()),
              SCORE_REJECT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathUnalignedBufferOffsetsParam,
                         Values(-DEFAULT_MEM_ALIGN + 1, 1, DEFAULT_MEM_ALIGN - 1));

struct FastpathIoParam : public FastpathTestBase, public TestWithParam<IoType> {

    void expect_io()
    {
        EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));
        EXPECT_CALL(*mbuffer, getLength).WillOnce(Return(DEFAULT_BUFFER_LENGTH));
        EXPECT_CALL(*mfile, getUnbufferedFd).WillOnce(Return(DEFAULT_FILE_DESCRIPTOR));
    }

    void expect_io(int fd, void *bufptr, size_t buflen)
    {
        EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(bufptr));
        EXPECT_CALL(*mbuffer, getLength).WillOnce(Return(buflen));
        EXPECT_CALL(*mfile, getUnbufferedFd).WillOnce(Return(fd));
    }
};

TEST_P(FastpathIoParam, IoRejectsNegativeFileOffset)
{
    expect_io();
    ASSERT_THROW(Fastpath().io(GetParam(), mfile, mbuffer, 0, -1, 0), std::invalid_argument);
}

TEST_P(FastpathIoParam, IoRejectsNegativeBufferOffset)
{
    expect_io();
    ASSERT_THROW(Fastpath().io(GetParam(), mfile, mbuffer, DEFAULT_IO_SIZE, 0, -1), std::invalid_argument);
}

TEST_P(FastpathIoParam, IoRejectsBufferOffsetLargerThanBufferLength)
{
    expect_io();
    ASSERT_THROW(Fastpath().io(GetParam(), mfile, mbuffer, DEFAULT_IO_SIZE, 0,
                               static_cast<hoff_t>(DEFAULT_BUFFER_LENGTH) + 1),
                 std::invalid_argument);
}

TEST_P(FastpathIoParam, IoRejectsIoSizeLargerThanBufferLength)
{
    expect_io();
    ASSERT_THROW(Fastpath().io(GetParam(), mfile, mbuffer, DEFAULT_BUFFER_LENGTH + 1, 0, 0),
                 std::invalid_argument);
}

TEST_P(FastpathIoParam, IoRejectsIoThatCouldOverflowBuffer)
{
    expect_io();
    ASSERT_THROW(Fastpath().io(GetParam(), mfile, mbuffer, DEFAULT_BUFFER_LENGTH, DEFAULT_FILE_OFFSET, 1),
                 std::invalid_argument);
}

TEST_P(FastpathIoParam, IoConfiguresHandle)
{
    StrictMock<MHip> mhip;

    hipAmdFileHandle_t handle{};
    handle.fd = DEFAULT_FILE_DESCRIPTOR;

    expect_io();
    switch (GetParam()) {
        case IoType::Read:
            EXPECT_CALL(mhip, hipAmdFileRead(Eq(handle), _, _, _));
            break;
        case IoType::Write:
            EXPECT_CALL(mhip, hipAmdFileWrite(Eq(handle), _, _, _));
            break;
        default:
            FAIL() << "Invalid IoType";
    }

    Fastpath().io(GetParam(), mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET, DEFAULT_BUFFER_OFFSET);
}

TEST_P(FastpathIoParam, IoCalculatesCorrectDevicePointer)
{
    StrictMock<MHip> mhip;

    off_t buffer_offset{0x1000};
    void *buffer_addr{reinterpret_cast<void *>(0x20000)};
    void *expected_device_ptr{reinterpret_cast<void *>(0x21000)};

    expect_io(DEFAULT_FILE_DESCRIPTOR, buffer_addr, static_cast<size_t>(buffer_offset) + DEFAULT_IO_SIZE);
    switch (GetParam()) {
        case IoType::Read:
            EXPECT_CALL(mhip, hipAmdFileRead(_, expected_device_ptr, _, _));
            break;
        case IoType::Write:
            EXPECT_CALL(mhip, hipAmdFileWrite(_, expected_device_ptr, _, _));
            break;
        default:
            FAIL() << "Invalid IoType";
    }

    Fastpath().io(GetParam(), mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET, buffer_offset);
}

TEST_P(FastpathIoParam, IoPassesThroughSizeAndFileOffset)
{
    StrictMock<MHip> mhip;

    expect_io();
    switch (GetParam()) {
        case IoType::Read:
            EXPECT_CALL(mhip, hipAmdFileRead(_, _, Eq(DEFAULT_IO_SIZE), Eq(DEFAULT_FILE_OFFSET)));
            break;
        case IoType::Write:
            EXPECT_CALL(mhip, hipAmdFileWrite(_, _, Eq(DEFAULT_IO_SIZE), Eq(DEFAULT_FILE_OFFSET)));
            break;
        default:
            FAIL() << "Invalid IoType";
    }

    Fastpath().io(GetParam(), mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET, DEFAULT_BUFFER_OFFSET);
}

TEST_P(FastpathIoParam, IoReturnsBytesTransferred)
{
    StrictMock<MHip> mhip;

    expect_io();
    switch (GetParam()) {
        case IoType::Read:
            EXPECT_CALL(mhip, hipAmdFileRead(_, _, _, _)).WillOnce(Return(DEFAULT_IO_SIZE));
            break;
        case IoType::Write:
            EXPECT_CALL(mhip, hipAmdFileWrite(_, _, _, _)).WillOnce(Return(DEFAULT_IO_SIZE));
            break;
        default:
            FAIL() << "Invalid IoType";
    }

    ASSERT_EQ(Fastpath().io(GetParam(), mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET,
                            DEFAULT_BUFFER_OFFSET),
              DEFAULT_IO_SIZE);
}

TEST_P(FastpathIoParam, IoReturnsBytesTransferredShort)
{
    StrictMock<MHip> mhip;

    size_t nbytes{4096};

    ASSERT_LT(nbytes, DEFAULT_IO_SIZE);

    expect_io();
    switch (GetParam()) {
        case IoType::Read:
            EXPECT_CALL(mhip, hipAmdFileRead(_, _, _, _)).WillOnce(Return(nbytes));
            break;
        case IoType::Write:
            EXPECT_CALL(mhip, hipAmdFileWrite(_, _, _, _)).WillOnce(Return(nbytes));
            break;
        default:
            FAIL() << "Invalid IoType";
    }

    ASSERT_EQ(Fastpath().io(GetParam(), mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET,
                            DEFAULT_BUFFER_OFFSET),
              nbytes);
}

// Ensure hip errors thrown by Hip::hipAmdFileRead/Hip::hipAmdFileWrite are not masked
TEST_P(FastpathIoParam, IoDoesNotMaskHipRuntimeError)
{
    StrictMock<MHip> mhip;

    expect_io();
    switch (GetParam()) {
        case IoType::Read:
            EXPECT_CALL(mhip, hipAmdFileRead).WillOnce(Throw(Hip::RuntimeError(hipErrorUnknown)));
            break;
        case IoType::Write:
            EXPECT_CALL(mhip, hipAmdFileWrite).WillOnce(Throw(Hip::RuntimeError(hipErrorUnknown)));
            break;
        default:
            FAIL() << "Invalid IoType";
    }

    EXPECT_THROW(Fastpath().io(GetParam(), mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET,
                               DEFAULT_BUFFER_OFFSET),
                 Hip::RuntimeError);
}

// Ensure hip errors thrown by Hip::hipAmdFileRead/Hip::hipAmdFileWrite are not masked
TEST_P(FastpathIoParam, IoDoesNotMaskSystemError)
{
    StrictMock<MHip> mhip;

    expect_io();
    switch (GetParam()) {
        case IoType::Read:
            EXPECT_CALL(mhip, hipAmdFileRead).WillOnce(Throw(system_error(ENODEV, generic_category())));
            break;
        case IoType::Write:
            EXPECT_CALL(mhip, hipAmdFileWrite).WillOnce(Throw(system_error(EINVAL, generic_category())));
            break;
        default:
            FAIL() << "Invalid IoType";
    }

    EXPECT_THROW(Fastpath().io(GetParam(), mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET,
                               DEFAULT_BUFFER_OFFSET),
                 std::system_error);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathIoParam, Values(IoType::Read, IoType::Write));

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
