/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "hipfile-test.h"
#include "mstats.h"
#include "stats.h"

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

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
