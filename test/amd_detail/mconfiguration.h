/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "configuration.h"

#include <gmock/gmock.h>

namespace rocFile {

struct MConfiguration : IConfiguration {
    MOCK_METHOD(bool, fastpath, (), (const, noexcept, override));
    MOCK_METHOD(bool, fallback, (), (const, noexcept, override));
    MOCK_METHOD(unsigned int, statsLevel, (), (const, noexcept, override));
};

}
