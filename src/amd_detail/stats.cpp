/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "context.h"
#include "stats.h"
#include "sys.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

namespace hipFile {
StatsServer::StatsServer()
    : m_fd{FileDescriptor::make_managed(Context<Sys>::get()->memfd_create("AISSTATS", MFD_ALLOW_SEALING))},
      m_stats{nullptr, &statsDeleter}
{
    int fd{m_fd.get()};
    Context<Sys>::get()->ftruncate(fd, sizeof(Stats));
    void *shm = Context<Sys>::get()->mmap(nullptr, sizeof(Stats), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    Context<Sys>::get()->fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_FUTURE_WRITE);
    m_stats = UniqueStats{new (shm) Stats{}, &statsDeleter};
}

StatsServer::~StatsServer()
{
}

void
StatsServer::statsDeleter(Stats *s)
{
    if (s == nullptr) {
        return;
    }
    s->~Stats();
    Context<Sys>::get()->munmap(s, sizeof(Stats));
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
