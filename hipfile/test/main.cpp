/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "test-common.h"

#include "hipfile-warnings.h"
#include "invalid-enum.h"

#include <gtest/gtest.h>
#include <hip/hip_runtime.h>

#include <ctime>

using namespace std;

// Put tests inside the macros to suppress the global constructor
// warnings
HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

TEST(Helpers, invalidEnum)
{
    enum foo {
        a = -7,
        b = 12,
    };
    ASSERT_EQ(invalidEnum<foo>(a - 1), -8);
    ASSERT_EQ(invalidEnum<foo>(a + 1), -6);
    ASSERT_EQ(invalidEnum<foo>(b - 1), 11);
    ASSERT_EQ(invalidEnum<foo>(b + 1), 13);
}

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
