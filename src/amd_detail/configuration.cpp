/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "configuration.h"
#include "environment.h"
#include "hip.h"
#include "static.h"

#include <optional>

using namespace hipFile;

bool
Configuration::fastpath() const noexcept
{
    STATIC bool fastpath_env{!Environment::force_compat_mode().value_or(false)};
    STATIC bool readExists{!!getHipAmdFileReadPtr()};
    STATIC bool writeExists{!!getHipAmdFileWritePtr()};
    return readExists && writeExists && fastpath_env;
}

bool
Configuration::fallback() const noexcept
{
    STATIC bool fallback_env{Environment::allow_compat_mode().value_or(true)};
    return fallback_env;
}

unsigned int
Configuration::statsLevel() const noexcept
{
    STATIC unsigned int stats_level_env{Environment::stats_level().value_or(0)};
    return stats_level_env;
}
