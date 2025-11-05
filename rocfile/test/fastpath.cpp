/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "mbuffer.h"
#include "mfile.h"
#include "mhip.h"
#include "rocfile-test.h"

#include "backend/fastpath.h"
#include "mountinfo.h"
#include "buffer.h"
#include "file.h"
#include "hip.h"
#include "io.h"

#include <optional>

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

// Provide defaut values for variables used in fastpath tests
struct FastpathTestBase {
    const size_t        DEFAULT_IO_SIZE{1024 * 1024};
    void *const         DEFAULT_BUFFER_ADDR{reinterpret_cast<void *>(0xCAFE000)};
    const off_t         DEFAULT_BUFFER_OFFSET{4096};
    const hipMemoryType DEFAULT_BUFFER_TYPE{hipMemoryTypeDevice};
    const int           DEFAULT_FILE_DESCRIPTOR{7};
    const int           DEFAULT_FILE_FLAGS{O_DIRECT};
    const off_t         DEFAULT_FILE_OFFSET{8192};

    // Buffer and file mocks used to setup expectations
    shared_ptr<StrictMock<MFile>>   mfile{make_shared<StrictMock<MFile>>()};
    shared_ptr<StrictMock<MBuffer>> mbuffer{make_shared<StrictMock<MBuffer>>()};
};

struct FastpathTest : public FastpathTestBase, public Test {};

TEST_F(FastpathTest, DefaultExpecations)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET, DEFAULT_BUFFER_OFFSET),
              SCORE_ACCEPT);
}

TEST_F(FastpathTest, BufferedFile)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS & ~O_DIRECT));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET, DEFAULT_BUFFER_OFFSET),
              SCORE_REJECT);
}

struct FastpathSupportedHipMemoryParam : public FastpathTestBase, public TestWithParam<hipMemoryType> {};

TEST_P(FastpathSupportedHipMemoryParam, Score)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(GetParam()));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET, DEFAULT_BUFFER_OFFSET),
              SCORE_ACCEPT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathSupportedHipMemoryParam, ValuesIn(SupportedHipMemoryTypes));

struct FastpathUnsupportedHipMemoryParam : public FastpathTestBase, public TestWithParam<hipMemoryType> {};

TEST_P(FastpathUnsupportedHipMemoryParam, Score)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(GetParam()));

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

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, GetParam(), DEFAULT_FILE_OFFSET, DEFAULT_BUFFER_OFFSET),
              SCORE_ACCEPT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathAlignedIoSizesParam,
                         Values(0, 4096, 1024 * 1024, 1024 * 1024 * 1024));

struct FastpathUnalignedIoSizesParam : public FastpathTestBase, public TestWithParam<size_t> {};

TEST_P(FastpathUnalignedIoSizesParam, Score)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, GetParam(), DEFAULT_FILE_OFFSET, DEFAULT_BUFFER_OFFSET),
              SCORE_REJECT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathUnalignedIoSizesParam,
                         Values(1, 4096 - 1, 1024 * 1024 + 1, 1024 * 1024 * 1024 - 1));

struct FastpathAlignedFileOffsetsParam : public FastpathTestBase, public TestWithParam<off_t> {};

TEST_P(FastpathAlignedFileOffsetsParam, Score)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, GetParam(), DEFAULT_BUFFER_OFFSET),
              SCORE_ACCEPT);
}

static array<off_t, 4> aligned_offsets{0, 4096, 1024 * 1024, 1024 * 1024 * 1024};
INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathAlignedFileOffsetsParam, ValuesIn(aligned_offsets));

/// @brief Tests negative and unaligned file offsets
struct FastpathInvalidFileOffsetsParam : public FastpathTestBase, public TestWithParam<off_t> {};

TEST_P(FastpathInvalidFileOffsetsParam, Score)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, GetParam(), DEFAULT_BUFFER_OFFSET),
              SCORE_REJECT);
}

static array<off_t, 7> invalid_offsets{-1024 * 1024 * 1024, -1024 * 1024,          -4096, 1, 4096 - 1,
                                       1024 * 1024 + 1,     1024 * 1024 * 1024 - 1};
INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathInvalidFileOffsetsParam, ValuesIn(invalid_offsets));

struct FastpathAlignedBufferOffsetsParam : public FastpathTestBase, public TestWithParam<off_t> {};

TEST_P(FastpathAlignedBufferOffsetsParam, Score)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_BUFFER_OFFSET, GetParam()),
              SCORE_ACCEPT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathAlignedBufferOffsetsParam, ValuesIn(aligned_offsets));

/// @brief Tests negative and unaligned buffer offsets
struct FastpathInvalidBufferOffsetsParam : public FastpathTestBase, public TestWithParam<off_t> {};

TEST_P(FastpathInvalidBufferOffsetsParam, Score)
{
    EXPECT_CALL(*mfile, getStatusFlags).WillOnce(Return(DEFAULT_FILE_FLAGS));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET, GetParam()),
              SCORE_REJECT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathInvalidBufferOffsetsParam, ValuesIn(invalid_offsets));

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

// Ensure hip errors thrown by Hip::hipAmdFileRead/Hip::hipAmdFileWtite are not masked
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

// Ensure hip errors thrown by Hip::hipAmdFileRead/Hip::hipAmdFileWtite are not masked
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
