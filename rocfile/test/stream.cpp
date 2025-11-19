/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "hipfile-warnings.h"
#include "mhip.h"
#include "msys.h"
#include "rocfile.h"
#include "rocfile-test.h"
#include "stream.h"

#include <algorithm>
#include <cstdint>
#include <gtest/gtest.h>
#include <hip/hip_runtime_api.h>
#include <memory>
#include <stdexcept>
#include <tuple>

using namespace rocFile;

using ::testing::StrictMock;

// Put tests inside the macros to suppress the global constructor
// warnings
HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

static auto
rocFileFlagsPowerSet()
{
    return ::testing::Combine(::testing::Values(0, ROCFILE_STREAM_FIXED_BUF_OFFSET),
                              ::testing::Values(0, ROCFILE_STREAM_FIXED_FILE_OFFSET),
                              ::testing::Values(0, ROCFILE_STREAM_FIXED_FILE_SIZE),
                              ::testing::Values(0, ROCFILE_STREAM_PAGE_ALIGNED_INPUTS));
}

struct RocFileStream : public ::testing::Test {
    RocFileStream()
    {
        nonnull_stream = reinterpret_cast<hipStream_t>(1);
    }
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;
    hipStream_t      nonnull_stream;
    StreamMap        stream_map;
};

struct RocFileStreamValidParams
    : public RocFileStream,
      public ::testing::WithParamInterface<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>> {
protected:
    RocFileStreamValidParams() : RocFileStream{}
    {
        auto params = GetParam();
        flags       = std::get<0>(params) | std::get<1>(params) | std::get<2>(params) | std::get<3>(params);
    }
    unsigned flags;
};

TEST_P(RocFileStreamValidParams, register_stream_valid_flags_internal)
{
    stream_map.registerStream(nonnull_stream, flags);
    auto stream = stream_map.getStream(nonnull_stream);
    ASSERT_EQ(stream->getHipStream(), nonnull_stream);
}

INSTANTIATE_TEST_SUITE_P(StreamSuite, RocFileStreamValidParams, rocFileFlagsPowerSet());

TEST_F(RocFileStream, get_stream_with_unregistered_stream_works)
{
    auto stream = stream_map.getStream(nonnull_stream);
    ASSERT_EQ(nonnull_stream, stream->getHipStream());
}

TEST_F(RocFileStream, register_with_invalid_flags_throws)
{
    ASSERT_THROW(stream_map.registerStream(nonnull_stream, ROCFILE_STREAM_FLAGS_MASK + 1),
                 std::invalid_argument);
}

TEST_F(RocFileStream, register_twice_throws)
{
    stream_map.registerStream(nonnull_stream, 0);
    ASSERT_THROW(stream_map.registerStream(nonnull_stream, 0), std::invalid_argument);
}

TEST_F(RocFileStream, deregister_with_registered_stream_works)
{
    stream_map.registerStream(nonnull_stream, 0);
    stream_map.deregisterStream(nonnull_stream);
}

TEST_F(RocFileStream, deregister_twice_throws)
{
    stream_map.registerStream(nonnull_stream, 0);
    stream_map.deregisterStream(nonnull_stream);
    ASSERT_THROW(stream_map.deregisterStream(nonnull_stream), std::invalid_argument);
}

TEST_F(RocFileStream, deregister_with_unregistered_stream_throws)
{
    ASSERT_THROW(stream_map.deregisterStream(nonnull_stream), std::invalid_argument);
}

TEST(RocFileStreamDestructor, destructor_with_streams_in_use_logs)
{
    StrictMock<MHip>         mhip;
    StrictMock<MSys>         msys;
    std::shared_ptr<IStream> stream;
    {
        StreamMap stream_map;
        stream_map.registerStream(reinterpret_cast<hipStream_t>(1), 0);
        EXPECT_CALL(msys, syslog);
        stream = stream_map.getStream(reinterpret_cast<hipStream_t>(1));
    }
}

struct RocFileStreamExternal : public RocFileOpened {
    RocFileStreamExternal()
    {
        nonnull_stream = reinterpret_cast<hipStream_t>(1);
    }
    StrictMock<MHip> mhip;
    StrictMock<MSys> msys;
    hipStream_t      nonnull_stream;
};

TEST_F(RocFileStreamExternal, register_and_deregister_with_valid_stream_works)
{
    ASSERT_EQ(rocFileStreamRegister(nonnull_stream, 0), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileStreamDeregister(nonnull_stream), ROCFILE_SUCCESS);
}

TEST_F(RocFileStreamExternal, deregister_exception_returns_error)
{
    ASSERT_EQ(rocFileStreamDeregister(nullptr), RocFileOpError(rocFileInvalidValue));
}

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
