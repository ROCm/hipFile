/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "context.h"
#include "stats.h"

namespace hipFile {
StatsServer::StatsServer()
{
}

StatsServer::~StatsServer()
{
}

void
statsAddFastPathRead(uint64_t bytes)
{
    Stats *stats{Context<StatsServer>::get()->getStats()};
    if (stats) {
        stats->getCounter(StatsCounters::TotalFastPathReadBytes) += bytes;
    }
}

void
statsAddFastPathWrite(uint64_t bytes)
{
    Stats *stats{Context<StatsServer>::get()->getStats()};
    if (stats) {
        stats->getCounter(StatsCounters::TotalFastPathWriteBytes) += bytes;
    }
}

void
statsAddFallbackPathRead(uint64_t bytes)
{
    Stats *stats{Context<StatsServer>::get()->getStats()};
    if (stats) {
        stats->getCounter(StatsCounters::TotalFallbackPathReadBytes) += bytes;
    }
}

void
statsAddFallbackPathWrite(uint64_t bytes)
{
    Stats *stats{Context<StatsServer>::get()->getStats()};
    if (stats) {
        stats->getCounter(StatsCounters::TotalFallbackPathWriteBytes) += bytes;
    }
}
}
