/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "backend/fastpath.h"
#include "hip.h"
#include "hipfile-warnings.h"
#include "io.h"
#include "mbuffer.h"
#include "mfile.h"
#include "mhip.h"
#include "rocfile-test.h"

#include <array>
#include <cerrno>
#include <fcntl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hip/hip_runtime_api.h>
#include <memory>
#include <sys/types.h>
#include <system_error>

using namespace rocFile;
using namespace testing;
using namespace std;

HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

static int       SCORE_ACCEPT{100};
static const int SCORE_REJECT{-1};

namespace rocFile {
inline bool
operator==(const hipAmdFileHandle_t &lhs, const hipAmdFileHandle_t &rhs)
{
    return lhs.fd == rhs.fd;
}
}

// Provide default values for variables used in fastpath tests
struct FastpathTestBase {
    const size_t              DEFAULT_IO_SIZE{1024 * 1024};
    void *const               DEFAULT_BUFFER_ADDR{reinterpret_cast<void *>(0x600DB10C)};
    const off_t               DEFAULT_BUFFER_OFFSET{512};
    const hipMemoryType       DEFAULT_BUFFER_TYPE{hipMemoryTypeDevice};
    const int                 DEFAULT_FILE_DESCRIPTOR{7};
    const int                 DEFAULT_FILE_FLAGS{O_DIRECT};
    const off_t               DEFAULT_FILE_OFFSET{8192};
    static const struct statx DEFAULT_STATX;

    // Buffer and file mocks used to setup expectations
    shared_ptr<StrictMock<MFile>>   mfile{make_shared<StrictMock<MFile>>()};
    shared_ptr<StrictMock<MBuffer>> mbuffer{make_shared<StrictMock<MBuffer>>()};
};

const struct statx FastpathTestBase::DEFAULT_STATX = []() {
    struct statx stx;
    stx.stx_mask             = STATX_DIOALIGN;
    stx.stx_dio_mem_align    = 4;
    stx.stx_dio_offset_align = 512;
    return stx;
}();

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
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS & ~O_DIRECT));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE,
                               -static_cast<hoff_t>(DEFAULT_STATX.stx_dio_offset_align),
                               DEFAULT_BUFFER_OFFSET),
              SCORE_REJECT);
}

TEST_F(FastpathTest, ScoreRejectsNegativeAlignedBufferOffset)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS & ~O_DIRECT));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_BUFFER_OFFSET,
                               -static_cast<hoff_t>(DEFAULT_STATX.stx_dio_offset_align)),
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
                         Values(FastpathTestBase::DEFAULT_STATX.stx_dio_offset_align * 0,
                                FastpathTestBase::DEFAULT_STATX.stx_dio_offset_align * 1,
                                FastpathTestBase::DEFAULT_STATX.stx_dio_offset_align * 2,
                                FastpathTestBase::DEFAULT_STATX.stx_dio_offset_align * 3));

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
                         Values(FastpathTestBase::DEFAULT_STATX.stx_dio_offset_align * 0 + 1,
                                FastpathTestBase::DEFAULT_STATX.stx_dio_offset_align * 1 - 1,
                                FastpathTestBase::DEFAULT_STATX.stx_dio_offset_align * 2 + 1,
                                FastpathTestBase::DEFAULT_STATX.stx_dio_offset_align * 3 - 1));

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
                         Values(FastpathTestBase::DEFAULT_STATX.stx_dio_offset_align * 0,
                                FastpathTestBase::DEFAULT_STATX.stx_dio_offset_align * 1,
                                FastpathTestBase::DEFAULT_STATX.stx_dio_offset_align * 2,
                                FastpathTestBase::DEFAULT_STATX.stx_dio_offset_align * 3));

/// @brief Tests negative and unaligned file offsets
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
                         Values(FastpathTestBase::DEFAULT_STATX.stx_dio_offset_align * 0 + 1,
                                FastpathTestBase::DEFAULT_STATX.stx_dio_offset_align * 1 - 1,
                                FastpathTestBase::DEFAULT_STATX.stx_dio_offset_align * 2 + 1,
                                FastpathTestBase::DEFAULT_STATX.stx_dio_offset_align * 3 - 1));

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
                         Values(FastpathTestBase::DEFAULT_STATX.stx_dio_mem_align * 0,
                                FastpathTestBase::DEFAULT_STATX.stx_dio_mem_align * 1,
                                FastpathTestBase::DEFAULT_STATX.stx_dio_mem_align * 2,
                                FastpathTestBase::DEFAULT_STATX.stx_dio_mem_align * 3));

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
                         Values(FastpathTestBase::DEFAULT_STATX.stx_dio_mem_align * 0 + 1,
                                FastpathTestBase::DEFAULT_STATX.stx_dio_mem_align * 1 - 1,
                                FastpathTestBase::DEFAULT_STATX.stx_dio_mem_align * 2 + 1,
                                FastpathTestBase::DEFAULT_STATX.stx_dio_mem_align * 3 - 1));

struct FastpathIoParam : public FastpathTestBase, public TestWithParam<IoType> {};

TEST_P(FastpathIoParam, IoConfiguresHandle)
{
    StrictMock<MHip> mhip;

    hipAmdFileHandle_t handle{};
    handle.fd = DEFAULT_FILE_DESCRIPTOR;

    EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));
    EXPECT_CALL(*mfile, getFd).WillOnce(Return(DEFAULT_FILE_DESCRIPTOR));
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

    EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(buffer_addr));
    EXPECT_CALL(*mfile, getFd).WillOnce(Return(DEFAULT_FILE_DESCRIPTOR));
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

    EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));
    EXPECT_CALL(*mfile, getFd).WillOnce(Return(DEFAULT_FILE_DESCRIPTOR));
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

    EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));
    EXPECT_CALL(*mfile, getFd).WillOnce(Return(DEFAULT_FILE_DESCRIPTOR));
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

    EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));
    EXPECT_CALL(*mfile, getFd).WillOnce(Return(DEFAULT_FILE_DESCRIPTOR));
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

    EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));
    EXPECT_CALL(*mfile, getFd).WillOnce(Return(DEFAULT_FILE_DESCRIPTOR));
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

    EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));
    EXPECT_CALL(*mfile, getFd).WillOnce(Return(DEFAULT_FILE_DESCRIPTOR));
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
