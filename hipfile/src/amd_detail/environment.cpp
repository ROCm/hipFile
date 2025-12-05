/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#include "environment.h"

using namespace hipFile;
using namespace std;

template <>
std::optional<bool>
Environment::get<bool>(const char *key)
{
    const char *value{Context<Sys>::get()->getenv(key)};
    if (value) {
        if (!strcasecmp(value, "true")) {
            return true;
        }
        if (!strcasecmp(value, "false")) {
            return false;
        }
    }
    return std::nullopt;
}

optional<bool>
Environment::allow_compat_mode()
{
    return Environment::get<bool>(Environment::ALLOW_COMPAT_MODE);
}
