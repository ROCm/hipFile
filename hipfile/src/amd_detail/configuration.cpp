/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "configuration.h"
#include "environment.h"
#include "hip.h"

using namespace hipFile;

Configuration::Configuration() : m_fastpath(true), m_fallback(true)
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
