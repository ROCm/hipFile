/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "hipfile-test.h"
#include "mconfiguration.h"
#include "mstats.h"
#include "stats.h"
#include "msys.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sstream>

using namespace hipFile;

using ::testing::StrictMock;

// Put tests inside the macros to suppress the global constructor
// warnings
HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

struct HipFileStats : public HipFileUnopened {};

TEST_F(HipFileStats, StatsCollectionAddIo)
{
    Stats                    stats{};
    StrictMock<MStatsServer> mstats{};
    EXPECT_CALL(mstats, getStats).WillRepeatedly(testing::Return(&stats));
    stats.level = StatsLevel::Basic;
    Context<StatsCollection>::get()->addIo(IoType::Read, StatsBackend::Fastpath, 10, 0);
    ASSERT_EQ(10, stats.getCounter(StatsCounters::TotalFastPathReadBytes).load());
    Context<StatsCollection>::get()->addIo(IoType::Write, StatsBackend::Fallback, 10, 0);
    ASSERT_EQ(10, stats.getCounter(StatsCounters::TotalFallbackPathWriteBytes).load());
    Context<StatsCollection>::get()->addIo(IoType::Read, StatsBackend::Fastpath, 10, 0);
    ASSERT_EQ(20, stats.getCounter(StatsCounters::TotalFastPathReadBytes).load());
    stats.level = StatsLevel::Disabled;
    Context<StatsCollection>::get()->addIo(IoType::Write, StatsBackend::Fallback, 10, 0);
    ASSERT_EQ(10, stats.getCounter(StatsCounters::TotalFallbackPathWriteBytes).load());
}

TEST_F(HipFileStats, StatsContainer)
{
    StrictMock<MSys>           msys{};
    StrictMock<MConfiguration> mcfg{};
    alignas(Stats) char        buff[sizeof(Stats)];
    EXPECT_CALL(msys, memfd_create).WillOnce(testing::Return(10));
    EXPECT_CALL(msys, fcntl).WillOnce(testing::Return(0));
    EXPECT_CALL(msys, ftruncate);
    EXPECT_CALL(msys, mmap).WillOnce(testing::Return(&buff));
    EXPECT_CALL(msys, munmap).Times(1);
    EXPECT_CALL(msys, close).Times(1);
    EXPECT_CALL(mcfg, statsLevel()).WillOnce(testing::Return(1));
    StatsContainer sc{};
    ASSERT_EQ(reinterpret_cast<Stats *>(&buff), sc.getStats());
    ASSERT_EQ(10, sc.getFd());
}

TEST_F(HipFileStats, GenerateReportV1)
{
    Stats              stats{};
    std::ostringstream os{};
    stats.getCounter(StatsCounters::TotalFastPathReadBytes)      = 2;
    stats.getCounter(StatsCounters::TotalFastPathWriteBytes)     = 4;
    stats.getCounter(StatsCounters::TotalFallbackPathReadBytes)  = 6;
    stats.getCounter(StatsCounters::TotalFallbackPathWriteBytes) = 8;
    StatsClient::generateReportV1(os, &stats);
    std::string str{os.str()};
    ASSERT_GT(std::string::npos, str.find('2'));
    ASSERT_GT(std::string::npos, str.find('4'));
    ASSERT_GT(std::string::npos, str.find('6'));
    ASSERT_GT(std::string::npos, str.find('8'));
}

struct HipFileStatsHistogram : public HipFileUnopened {};

TEST_F(HipFileStatsHistogram, ToHistogramBucket)
{
    ASSERT_EQ(0, StatsHistogram::toHistogramBucket(0));
    ASSERT_EQ(0, StatsHistogram::toHistogramBucket(1));
    ASSERT_EQ(0, StatsHistogram::toHistogramBucket(4095));
    ASSERT_EQ(1, StatsHistogram::toHistogramBucket(4096));
    ASSERT_EQ(1, StatsHistogram::toHistogramBucket(8191));
    ASSERT_EQ(2, StatsHistogram::toHistogramBucket(8192));
    ASSERT_EQ(2, StatsHistogram::toHistogramBucket(16383));
    ASSERT_EQ(3, StatsHistogram::toHistogramBucket(16384));
    ASSERT_EQ(14, StatsHistogram::toHistogramBucket(uint64_t{1} << 25));
    ASSERT_EQ(15, StatsHistogram::toHistogramBucket(uint64_t{1} << 30));
    ASSERT_EQ(15, StatsHistogram::toHistogramBucket(uint64_t{1} << 40));
}

TEST_F(HipFileStatsHistogram, BucketRange)
{
    auto [lower0, upper0] = StatsHistogram::bucketRange(0);
    ASSERT_EQ(0, lower0);
    ASSERT_EQ(4096, upper0);
    auto [lower1, upper1] = StatsHistogram::bucketRange(1);
    ASSERT_EQ(4096, lower1);
    ASSERT_EQ(8192, upper1);
    auto [lower2, upper2] = StatsHistogram::bucketRange(2);
    ASSERT_EQ(8192, lower2);
    ASSERT_EQ(16384, upper2);
    auto [lower3, upper3] = StatsHistogram::bucketRange(3);
    ASSERT_EQ(16384, lower3);
    ASSERT_EQ(32768, upper3);
    auto [lower14, upper14] = StatsHistogram::bucketRange(14);
    ASSERT_EQ(uint64_t{1} << 25, lower14);
    ASSERT_EQ(uint64_t{1} << 26, upper14);
    auto [lower15, upper15] = StatsHistogram::bucketRange(15);
    ASSERT_EQ(uint64_t{1} << 26, lower15);
    ASSERT_EQ(UINT64_MAX, upper15);
}

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
