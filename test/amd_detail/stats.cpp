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

TEST_F(HipFileStats, StatsServerLifetime)
{
    StrictMock<MSys>           msys{};
    StrictMock<MConfiguration> mcfg{};
    char                       buff[sizeof(Stats)];
    EXPECT_CALL(msys, memfd_create).WillOnce(testing::Return(10));
    EXPECT_CALL(msys, eventfd).WillOnce(testing::Return(11));
    EXPECT_CALL(msys, fcntl).WillOnce(testing::Return(0));
    EXPECT_CALL(msys, ftruncate);
    EXPECT_CALL(msys, mmap).WillOnce(testing::Return(&buff));
    EXPECT_CALL(msys, munmap);
    EXPECT_CALL(msys, close).Times(3);
    EXPECT_CALL(mcfg, statsLevel()).WillOnce(testing::Return(1));
    StatsServer srvr{};
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

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
