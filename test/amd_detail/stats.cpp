/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "hipfile-test.h"
#include "mstats.h"
#include "stats.h"
#include "msys.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace hipFile;

using ::testing::StrictMock;

// Put tests inside the macros to suppress the global constructor
// warnings
HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

struct HipFileStats : public HipFileUnopened {};

#define STAT_TEST(name)                                                                                      \
    TEST_F(HipFileStats, statsAdd##name)                                                                     \
    {                                                                                                        \
        Stats                    stats{};                                                                    \
        StrictMock<MStatsServer> mstats{};                                                                   \
        EXPECT_CALL(mstats, getStats).WillRepeatedly(testing::Return(&stats));                               \
        statsAdd##name(0x10);                                                                                \
        ASSERT_EQ(0x10, stats.getCounter(StatsCounters::Total##name##Bytes).load());                         \
        statsAdd##name(0x10);                                                                                \
        ASSERT_EQ(0x20, stats.getCounter(StatsCounters::Total##name##Bytes).load());                         \
    }

STAT_TEST(FastPathRead)
STAT_TEST(FastPathWrite)
STAT_TEST(FallbackPathRead)
STAT_TEST(FallbackPathWrite)

TEST_F(HipFileStats, StatsServerLifetime)
{
    StrictMock<MSys> msys{};
    char             buff[sizeof(Stats)];
    EXPECT_CALL(msys, memfd_create).WillOnce(testing::Return(10));
    EXPECT_CALL(msys, fcntl).WillOnce(testing::Return(0));
    EXPECT_CALL(msys, ftruncate);
    EXPECT_CALL(msys, mmap).WillOnce(testing::Return(&buff));
    EXPECT_CALL(msys, munmap);
    EXPECT_CALL(msys, close);
    StatsServer srvr{};
}

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
