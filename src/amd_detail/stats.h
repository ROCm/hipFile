/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "file-descriptor.h"
#include "io.h"

#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <numeric>
#include <ostream>
#include <thread>
#include <utility>

namespace hipFile {

enum class StatsLevel : uint64_t {
    Disabled,
    Basic,
    Detailed,
    Max,
};

enum class StatsBackend {
    Fastpath,
    Fallback,
    Count,
};

struct StatsHistogram {
    static constexpr size_t   MaxBuckets{16};
    static constexpr uint64_t FirstBucketBits{12};
    using Buckets = std::array<std::atomic_uint64_t, MaxBuckets>;
    Buckets buckets{};

    static size_t toHistogramBucket(uint64_t bytes) noexcept
    {
        bytes >>= FirstBucketBits;
        int idx{bytes == 0 ? 0 : 64 - __builtin_clzll(bytes)}; // log2(); clz(0) is UB
        return std::min(static_cast<size_t>(idx), MaxBuckets - 1);
    }

    static constexpr std::pair<uint64_t, uint64_t> bucketRange(size_t bucket) noexcept
    {
        if (bucket == 0) {
            return std::make_pair(uint64_t{0}, uint64_t{1} << FirstBucketBits);
        }
        else if (bucket < MaxBuckets - 1) {
            uint64_t lower{uint64_t{1} << (bucket + FirstBucketBits - 1)};
            return std::make_pair(lower, lower << 1);
        }
        else {
            return std::make_pair(uint64_t{1} << (MaxBuckets + FirstBucketBits - 2), UINT64_MAX);
        }
    }

    uint64_t accumulate() const noexcept
    {
        return std::accumulate(buckets.begin(), buckets.end(), uint64_t{0});
    }
};

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
    StatsLevel     level{};
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

class StatsContainer {
public:
    StatsContainer();
    ~StatsContainer();

    StatsContainer(const StatsContainer &)            = delete;
    StatsContainer &operator=(const StatsContainer &) = delete;

    StatsContainer(StatsContainer &&other) noexcept : m_fd(std::move(other.m_fd)), m_stats(other.m_stats)
    {
        other.m_stats = nullptr;
    }

    StatsContainer &operator=(StatsContainer &&other) noexcept
    {
        if (this != &other) {
            std::swap(m_fd, other.m_fd);
            std::swap(m_stats, other.m_stats);
        }
        return *this;
    }

    Stats *getStats() noexcept
    {
        return m_stats;
    }

    const Stats *getStats() const noexcept
    {
        return m_stats;
    }

    int getFd() const noexcept
    {
        return m_fd.get();
    }

private:
    FileDescriptor m_fd;
    Stats         *m_stats;
};

class StatsServer {
public:
    StatsServer();
    virtual ~StatsServer();
    virtual Stats *getStats() noexcept
    {
        return m_stats.getStats();
    }

private:
    void           threadFn();
    FileDescriptor m_efd;
    StatsContainer m_stats;
    std::thread    m_thread;
};

class IStatsClient {
public:
    virtual ~IStatsClient()                                 = default;
    virtual bool pollProcess(int timeout) const             = 0;
    virtual bool connectServer()                            = 0;
    virtual bool generateReport(std::ostream &stream) const = 0;
};

class StatsClient : public IStatsClient {
public:
    explicit StatsClient(pid_t p);
    bool pollProcess(int timeout) const override;
    bool connectServer() override;
    bool generateReport(std::ostream &stream) const override;

    static void generateReportV1(std::ostream &stream, const Stats *stats);

private:
    FileDescriptor m_pfd, m_sfd;
    pid_t          m_pid{0};
};

class StatsIoTracker {
public:
    StatsIoTracker(IoType ioType, StatsBackend backend) noexcept
        : m_ioType(ioType), m_backend(backend), m_startTime{std::chrono::steady_clock::now()}
    {
    }
    void complete(uint64_t bytes) const noexcept;

private:
    IoType                                m_ioType;
    StatsBackend                          m_backend;
    std::chrono::steady_clock::time_point m_startTime;
};

class StatsCollection {
public:
    virtual ~StatsCollection() = default;
    virtual void addIo(IoType ioType, StatsBackend backend, uint64_t bytes, uint64_t timeUs) const noexcept;
};
}
