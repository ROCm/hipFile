/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "configuration.h"
#include "environment.h"
#include "hip.h"

#include <optional>

using namespace hipFile;

Configuration::Configuration() : m_fastpath(true), m_fallback(true), m_statsLevel(0)
{
    auto maybe_env_force_compat{Environment::force_compat_mode()};
    if (maybe_env_force_compat && maybe_env_force_compat.value()) {
        m_fastpath = false;
    }
    m_fastpath = !!getHipAmdFileReadPtr() && m_fastpath;
    m_fastpath = !!getHipAmdFileWritePtr() && m_fastpath;

    auto maybe_env_allow_compat{Environment::allow_compat_mode()};
    if (maybe_env_allow_compat && !maybe_env_allow_compat.value()) {
        m_fallback = false;
    }

    auto maybe_stats_level{Environment::stats_level()};
    if (maybe_stats_level) {
        m_statsLevel = maybe_stats_level.value();
    }
}

bool
Configuration::fastpath() const noexcept
{
    return m_fastpath;
}

bool
Configuration::fallback() const noexcept
{
    return m_fallback;
}

unsigned int
Configuration::statsLevel() const noexcept
{
    return m_statsLevel;
}
