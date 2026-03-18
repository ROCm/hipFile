/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "backend/fallback.h"
#include "backend/fastpath.h"
#include "hip.h"
#include "hipfile.h"
#include "hipfile-test.h"
#include "hipfile-warnings.h"
#include "io.h"
#include "mbuffer.h"
#include "mfile.h"
#include "mhip.h"
#include "msys.h"

#include <array>
#include <cerrno>
#include <cstdint>
#include <exception>
#include <fcntl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hip/hip_runtime_api.h>
#include <linux/stat.h>
#include <memory>
#include <stdexcept>
#include <sys/types.h>
#include <system_error>
#include <tuple>

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
    void *const         DEFAULT_BUFFER_ADDR{reinterpret_cast<void *>(0xABAD'CAFE'0000'0000)};
    const off_t         DEFAULT_BUFFER_OFFSET{DEFAULT_MEM_ALIGN};
    const size_t        DEFAULT_BUFFER_LENGTH{DEFAULT_IO_SIZE + static_cast<size_t>(DEFAULT_BUFFER_OFFSET)};
    const hipMemoryType DEFAULT_BUFFER_TYPE{hipMemoryTypeDevice};
    const optional<int> DEFAULT_UNBUFFERED_FD{7};
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

TEST_F(FastpathTest, TestDefaults)
{
    ASSERT_FALSE((DEFAULT_MEM_ALIGN & (DEFAULT_MEM_ALIGN - 1)));
    ASSERT_TRUE(DEFAULT_MEM_ALIGN > 1);
    ASSERT_FALSE((DEFAULT_OFFSET_ALIGN & (DEFAULT_OFFSET_ALIGN - 1)));
    ASSERT_TRUE(DEFAULT_OFFSET_ALIGN > 1);
    ASSERT_FALSE((reinterpret_cast<uintptr_t>(DEFAULT_BUFFER_ADDR) & (DEFAULT_MEM_ALIGN - 1)));
    ASSERT_FALSE((DEFAULT_BUFFER_OFFSET & (DEFAULT_MEM_ALIGN - 1)));
    ASSERT_FALSE((DEFAULT_IO_SIZE & (DEFAULT_OFFSET_ALIGN - 1)));
    ASSERT_FALSE((DEFAULT_FILE_OFFSET & (DEFAULT_OFFSET_ALIGN - 1)));
}

TEST_F(FastpathTest, UnbufferedFdAvailable)
{
    EXPECT_CALL(*mfile, getUnbufferedFd).WillOnce(Return(DEFAULT_UNBUFFERED_FD));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
#if defined(STATX_DIOALIGN)
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));
#endif
    EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET, DEFAULT_BUFFER_OFFSET),
              SCORE_ACCEPT);
}

TEST_F(FastpathTest, UnbufferedFdNotAvailable)
{
    EXPECT_CALL(*mfile, getUnbufferedFd).WillOnce(Return(nullopt));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
#if defined(STATX_DIOALIGN)
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));
#endif
    EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET, DEFAULT_BUFFER_OFFSET),
              SCORE_REJECT);
}

TEST_F(FastpathTest, ScoreRejectsNegativeAlignedFileOffset)
{
    EXPECT_CALL(*mfile, getUnbufferedFd).WillOnce(Return(DEFAULT_UNBUFFERED_FD));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
#if defined(STATX_DIOALIGN)
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));
#endif
    EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, -static_cast<hoff_t>(DEFAULT_OFFSET_ALIGN),
                               DEFAULT_BUFFER_OFFSET),
              SCORE_REJECT);
}

TEST_F(FastpathTest, ScoreRejectsNegativeAlignedBufferOffset)
{
    EXPECT_CALL(*mfile, getUnbufferedFd).WillOnce(Return(DEFAULT_UNBUFFERED_FD));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
#if defined(STATX_DIOALIGN)
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));
#endif
    EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET,
                               -static_cast<hoff_t>(DEFAULT_MEM_ALIGN)),
              SCORE_REJECT);
}

TEST_F(FastpathTest, ScoreRejectsBufferAddressPlusBufferOffsetIsUnaligned)
{
    EXPECT_CALL(*mfile, getUnbufferedFd).WillOnce(Return(DEFAULT_UNBUFFERED_FD));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
#if defined(STATX_DIOALIGN)
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));
#endif
    // The DEFAULT_BUFFER_ADDR is DEFAULT_MEM_ALIGN aligned. Ensure that this
    // test's buffer is not DEFAULT_MEM_ALIGN aligned.
    EXPECT_CALL(*mbuffer, getBuffer)
        .WillOnce(Return(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(DEFAULT_BUFFER_ADDR) +
                                                  (DEFAULT_MEM_ALIGN >> 1))));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET,
                               static_cast<hoff_t>(DEFAULT_MEM_ALIGN)),
              SCORE_REJECT);
}

struct FastpathSupportedHipMemoryParam : public FastpathTestBase, public TestWithParam<hipMemoryType> {};

TEST_P(FastpathSupportedHipMemoryParam, Score)
{
    EXPECT_CALL(*mfile, getUnbufferedFd).WillOnce(Return(DEFAULT_UNBUFFERED_FD));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(GetParam()));
#if defined(STATX_DIOALIGN)
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));
#endif
    EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET, DEFAULT_BUFFER_OFFSET),
              SCORE_ACCEPT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathSupportedHipMemoryParam, ValuesIn(SupportedHipMemoryTypes));

struct FastpathUnsupportedHipMemoryParam : public FastpathTestBase, public TestWithParam<hipMemoryType> {};

TEST_P(FastpathUnsupportedHipMemoryParam, Score)
{
    EXPECT_CALL(*mfile, getUnbufferedFd).WillOnce(Return(DEFAULT_UNBUFFERED_FD));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(GetParam()));
#if defined(STATX_DIOALIGN)
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));
#endif
    EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET, DEFAULT_BUFFER_OFFSET),
              SCORE_REJECT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathUnsupportedHipMemoryParam,
                         ValuesIn(UnsupportedHipMemoryTypes));

struct FastpathAlignedIoSizesParam : public FastpathTestBase, public TestWithParam<size_t> {};

TEST_P(FastpathAlignedIoSizesParam, Score)
{
    EXPECT_CALL(*mfile, getUnbufferedFd).WillOnce(Return(DEFAULT_UNBUFFERED_FD));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
#if defined(STATX_DIOALIGN)
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));
#endif
    EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, GetParam(), DEFAULT_FILE_OFFSET, DEFAULT_BUFFER_OFFSET),
              SCORE_ACCEPT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathAlignedIoSizesParam,
                         Values(0, DEFAULT_OFFSET_ALIGN, DEFAULT_OFFSET_ALIGN << 1,
                                DEFAULT_OFFSET_ALIGN << 2));

struct FastpathUnalignedIoSizesParam : public FastpathTestBase, public TestWithParam<size_t> {};

TEST_P(FastpathUnalignedIoSizesParam, Score)
{
    EXPECT_CALL(*mfile, getUnbufferedFd).WillOnce(Return(DEFAULT_UNBUFFERED_FD));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
#if defined(STATX_DIOALIGN)
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));
#endif
    EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, GetParam(), DEFAULT_FILE_OFFSET, DEFAULT_BUFFER_OFFSET),
              SCORE_REJECT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathUnalignedIoSizesParam,
                         Values(1, DEFAULT_OFFSET_ALIGN >> 1, DEFAULT_OFFSET_ALIGN - 1,
                                DEFAULT_OFFSET_ALIGN + 1));

struct FastpathAlignedFileOffsetsParam : public FastpathTestBase, public TestWithParam<hoff_t> {};

TEST_P(FastpathAlignedFileOffsetsParam, Score)
{
    EXPECT_CALL(*mfile, getUnbufferedFd).WillOnce(Return(DEFAULT_UNBUFFERED_FD));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
#if defined(STATX_DIOALIGN)
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));
#endif
    EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, GetParam(), DEFAULT_BUFFER_OFFSET),
              GetParam() >= 0 ? SCORE_ACCEPT : SCORE_REJECT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathAlignedFileOffsetsParam,
                         Values(-static_cast<hoff_t>(DEFAULT_OFFSET_ALIGN << 2),
                                -static_cast<hoff_t>(DEFAULT_OFFSET_ALIGN << 1),
                                -static_cast<hoff_t>(DEFAULT_OFFSET_ALIGN), 0, DEFAULT_OFFSET_ALIGN,
                                DEFAULT_OFFSET_ALIGN << 1, DEFAULT_OFFSET_ALIGN << 2));

struct FastpathUnalignedFileOffsetsParam : public FastpathTestBase, public TestWithParam<hoff_t> {};

TEST_P(FastpathUnalignedFileOffsetsParam, Score)
{
    EXPECT_CALL(*mfile, getUnbufferedFd).WillOnce(Return(DEFAULT_UNBUFFERED_FD));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
#if defined(STATX_DIOALIGN)
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));
#endif
    EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, GetParam(), DEFAULT_BUFFER_OFFSET),
              SCORE_REJECT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathUnalignedFileOffsetsParam,
                         Values(-static_cast<hoff_t>(DEFAULT_OFFSET_ALIGN << 2) - 1,
                                -static_cast<hoff_t>(DEFAULT_OFFSET_ALIGN << 1) + 1,
                                -static_cast<hoff_t>(DEFAULT_OFFSET_ALIGN) - 1,
                                -static_cast<hoff_t>(DEFAULT_OFFSET_ALIGN >> 1), 1, DEFAULT_OFFSET_ALIGN >> 1,
                                DEFAULT_OFFSET_ALIGN - 1, (DEFAULT_OFFSET_ALIGN << 1) + 1,
                                (DEFAULT_OFFSET_ALIGN << 2) - 1));

struct FastpathAlignedBufferOffsetsParam : public FastpathTestBase, public TestWithParam<hoff_t> {};

TEST_P(FastpathAlignedBufferOffsetsParam, Score)
{
    EXPECT_CALL(*mfile, getUnbufferedFd).WillOnce(Return(DEFAULT_UNBUFFERED_FD));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
#if defined(STATX_DIOALIGN)
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));
#endif
    EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET, GetParam()),
              GetParam() >= 0 ? SCORE_ACCEPT : SCORE_REJECT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathAlignedBufferOffsetsParam,
                         Values(-static_cast<hoff_t>(DEFAULT_MEM_ALIGN << 2),
                                -static_cast<hoff_t>(DEFAULT_MEM_ALIGN << 1),
                                -static_cast<hoff_t>(DEFAULT_MEM_ALIGN), 0, DEFAULT_MEM_ALIGN,
                                DEFAULT_MEM_ALIGN << 1, DEFAULT_MEM_ALIGN << 2));

/// @brief Tests negative and unaligned buffer offsets
struct FastpathUnalignedBufferOffsetsParam : public FastpathTestBase, public TestWithParam<hoff_t> {};

TEST_P(FastpathUnalignedBufferOffsetsParam, Score)
{
    EXPECT_CALL(*mfile, getUnbufferedFd).WillOnce(Return(DEFAULT_UNBUFFERED_FD));
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(DEFAULT_BUFFER_TYPE));
#if defined(STATX_DIOALIGN)
    EXPECT_CALL(*mfile, getStatx).WillOnce(ReturnRef(DEFAULT_STATX));
#endif
    EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));

    ASSERT_EQ(Fastpath().score(mfile, mbuffer, DEFAULT_IO_SIZE, DEFAULT_FILE_OFFSET, GetParam()),
              SCORE_REJECT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathUnalignedBufferOffsetsParam,
                         Values(-static_cast<hoff_t>(DEFAULT_MEM_ALIGN << 2) - 1,
                                -static_cast<hoff_t>(DEFAULT_MEM_ALIGN << 1) + 1,
                                -static_cast<hoff_t>(DEFAULT_MEM_ALIGN) - 1,
                                -static_cast<hoff_t>(DEFAULT_MEM_ALIGN >> 1), 1, DEFAULT_MEM_ALIGN >> 1,
                                DEFAULT_MEM_ALIGN - 1, (DEFAULT_MEM_ALIGN << 1) + 1,
                                (DEFAULT_MEM_ALIGN << 2) - 1));

struct FastpathIoParam : public FastpathTestBase, public TestWithParam<IoType> {

    void expect_io()
    {
        EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));
        EXPECT_CALL(*mbuffer, getLength).WillOnce(Return(DEFAULT_BUFFER_LENGTH));
        EXPECT_CALL(*mfile, getUnbufferedFd).WillOnce(Return(DEFAULT_UNBUFFERED_FD));
    }

    void expect_io(optional<int> fd, void *bufptr, size_t buflen)
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
    handle.fd = DEFAULT_UNBUFFERED_FD.value();

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

    expect_io(DEFAULT_UNBUFFERED_FD, buffer_addr, static_cast<size_t>(buffer_offset) + DEFAULT_IO_SIZE);
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

// Ensure IO size is truncated to MAX_RW_COUNT
TEST_P(FastpathIoParam, IoSizeIsTruncatedToMaxRWCount)
{
    StrictMock<MHip> mhip;

    const size_t buffer_size{SIZE_MAX};
    const size_t io_size{SIZE_MAX};

    expect_io(DEFAULT_UNBUFFERED_FD, DEFAULT_BUFFER_ADDR, buffer_size);
    switch (GetParam()) {
        case IoType::Read:
            EXPECT_CALL(mhip, hipAmdFileRead(_, _, MAX_RW_COUNT, _)).WillOnce(Return(MAX_RW_COUNT));
            break;
        case IoType::Write:
            EXPECT_CALL(mhip, hipAmdFileWrite(_, _, MAX_RW_COUNT, _)).WillOnce(Return(MAX_RW_COUNT));
            break;
        default:
            FAIL() << "Invalid IoType";
    }

    ASSERT_EQ(Fastpath().io(GetParam(), mfile, mbuffer, io_size, 0, 0), MAX_RW_COUNT);
}

INSTANTIATE_TEST_SUITE_P(FastpathTest, FastpathIoParam, Values(IoType::Read, IoType::Write));

struct FastpathIoParamWithFallback : public FastpathTestBase,
                                     public TestWithParam<std::tuple<IoType, std::exception_ptr>> {
    inline IoType _get_param_io_type() const
    {
        return std::get<0>(GetParam());
    }

    inline const std::exception_ptr _get_param_exc_ptr() const
    {
        return std::get<1>(GetParam());
    }
};

// The Fastpath can throw a few different kinds of derived std::runtime_errors.
TEST_P(FastpathIoParamWithFallback, IntegrationRunWithFallback)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    auto fallback_backend = std::make_shared<StrictMock<Fallback>>();
    auto fastpath_backend = std::make_shared<StrictMock<Fastpath>>();
    fastpath_backend->register_fallback_backend(fallback_backend);

    const int DEFAULT_BUFFERED_FD = DEFAULT_UNBUFFERED_FD.value() + 1;

    // Called by both Fastpath and Fallback
    EXPECT_CALL(*mbuffer, getBuffer).WillRepeatedly(Return(DEFAULT_BUFFER_ADDR));
    EXPECT_CALL(*mbuffer, getLength).Times(2).WillRepeatedly(Return(DEFAULT_BUFFER_LENGTH));
    // Called only by Fastpath
    EXPECT_CALL(*mfile, getUnbufferedFd).WillOnce(Return(DEFAULT_UNBUFFERED_FD));
    // Called only by Fallback
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(hipMemoryTypeDevice));
    EXPECT_CALL(*mfile, getBufferedFd).WillRepeatedly(Return(DEFAULT_BUFFERED_FD));
    EXPECT_CALL(mhip, hipMemcpy).WillRepeatedly(Return());
    EXPECT_CALL(msys, mmap).WillOnce(Return(reinterpret_cast<void *>(0x12345678)));
    EXPECT_CALL(msys, munmap).WillOnce(Return());
    switch (_get_param_io_type()) {
        case IoType::Read:
            // Called by Fastpath
            EXPECT_CALL(mhip, hipAmdFileRead).WillOnce(Rethrow(_get_param_exc_ptr()));
            // Called by Fallback
            EXPECT_CALL(msys, pread).WillRepeatedly(ReturnArg<2>());
            break;
        case IoType::Write:
            // Called by Fastpath
            EXPECT_CALL(mhip, hipAmdFileWrite).WillOnce(Rethrow(_get_param_exc_ptr()));
            // Called by Fallback
            EXPECT_CALL(mhip, hipStreamSynchronize).WillRepeatedly(Return());
            EXPECT_CALL(msys, fdatasync).WillRepeatedly(Return());
            EXPECT_CALL(msys, pwrite).WillRepeatedly(ReturnArg<2>());
            break;
        default:
            FAIL() << "Invalid IoType";
    }

    ssize_t num_bytes = fastpath_backend->io(_get_param_io_type(), mfile, mbuffer, DEFAULT_IO_SIZE, 0, 0);
    ASSERT_EQ(num_bytes, DEFAULT_IO_SIZE);
}

// If the fallback backend rejects the IO, the original exception from
// Fastpath should be raised.
TEST_P(FastpathIoParamWithFallback, IntegrationFallbackRejectsIO)
{
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;

    auto fallback_backend = std::make_shared<StrictMock<Fallback>>();
    auto fastpath_backend = std::make_shared<StrictMock<Fastpath>>();
    fastpath_backend->register_fallback_backend(fallback_backend);

    // Called only by Fastpath
    EXPECT_CALL(*mbuffer, getBuffer).WillOnce(Return(DEFAULT_BUFFER_ADDR));
    EXPECT_CALL(*mbuffer, getLength).WillOnce(Return(DEFAULT_BUFFER_LENGTH));
    EXPECT_CALL(*mfile, getUnbufferedFd).WillOnce(Return(DEFAULT_UNBUFFERED_FD));
    // Called only by Fallback - should fail the score() check.
    EXPECT_CALL(*mbuffer, getType).WillOnce(Return(hipMemoryTypeHost));
    switch (_get_param_io_type()) {
        case IoType::Read:
            // Called by Fastpath
            EXPECT_CALL(mhip, hipAmdFileRead).WillOnce(Rethrow(_get_param_exc_ptr()));
            break;
        case IoType::Write:
            // Called by Fastpath
            EXPECT_CALL(mhip, hipAmdFileWrite).WillOnce(Rethrow(_get_param_exc_ptr()));
            break;
        default:
            FAIL() << "Invalid IoType";
    }

    // Have to rethrow the exception_ptr to be able to access the exception
    // This looks ugly, but is better than the alternative of trying to preserve the
    // exception type when setting Throw(*std::shared_ptr<std::exception>>).
    try {
        std::rethrow_exception(_get_param_exc_ptr());
    }
    catch (const std::exception &expected_exc) {
        // Can't use EXPECT_THROW due to the thrown exception type only being known at runtime
        try {
            fastpath_backend->io(_get_param_io_type(), mfile, mbuffer, DEFAULT_IO_SIZE, 0, 0);
        }
        catch (const std::exception &actual_exc) {
            // Verify that the propagated exception has the same dynamic type and message
            // as the one stored in the original std::exception_ptr, without relying on
            // pointer identity of the underlying exception object.
            ASSERT_EQ(typeid(expected_exc), typeid(actual_exc));
            ASSERT_STREQ(expected_exc.what(), actual_exc.what());
        }
        catch (...) {
            FAIL() << "io() threw something other than a std::exception";
        }
    }
}

// Using std::exception_ptr is more straightforward here than storing a pointer
// to a derived std::exception type, which would require careful handling when
// setting expectations. Note that Throw() does not accept std::exception_ptr,
// but the public (though undocumented) Rethrow() action does support it.
INSTANTIATE_TEST_SUITE_P(
    FastpathTest, FastpathIoParamWithFallback,
    Combine(Values(IoType::Read, IoType::Write),
            Values(std::make_exception_ptr(Hip::RuntimeError(hipErrorNoDevice)),
                   std::make_exception_ptr(std::system_error(make_error_code(errc::no_such_device))))));

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
