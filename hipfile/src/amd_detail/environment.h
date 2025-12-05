/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "context.h"
#include "sys.h"

#include <cstring>
#include <optional>

namespace hipFile {

class Environment {
public:
    /// @brief Get a value from the environment
    /// @param key The name of the variable
    /// @return An optional value of type T if the key was set, nullopt if the key
    /// was not set or had an invalid value for the type T
    template <typename T> static std::optional<T> get(const char *key);
};

}
