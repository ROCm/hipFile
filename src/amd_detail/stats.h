/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <array>
#include <atomic>

namespace hipFile {

/// When adding new fields, remember to increment Stats::version
enum class StatsCounters {
    TotalFastPathReadBytes,
    TotalFastPathWriteBytes,
    TotalFallbackPathReadBytes,
    TotalFallbackPathWriteBytes,

    Max,
    Capacity = 64,
};

static_assert(StatsCounters::Max <= StatsCounters::Capacity, "Increase StatsCounters::Capacity");

struct Stats {
    using Array = std::array<std::atomic_uint64_t, static_cast<std::size_t>(StatsCounters::Capacity)>;
    const uint64_t version{1};
    Array          counters;

    std::atomic_uint64_t &getCounter(StatsCounters index)
    {
        return counters[static_cast<std::size_t>(index)];
    }
    const std::atomic_uint64_t &getCounter(StatsCounters index) const
    {
        return counters[static_cast<std::size_t>(index)];
    }
};

void statsAddFastPathRead(uint64_t bytes);
void statsAddFastPathWrite(uint64_t bytes);
void statsAddFallbackPathRead(uint64_t bytes);
void statsAddFallbackPathWrite(uint64_t bytes);

class StatsServer {
public:
    StatsServer();
    virtual ~StatsServer();
    virtual Stats *getStats()
    {
        return m_stats;
    }

private:
    Stats *m_stats{nullptr};
};
}
