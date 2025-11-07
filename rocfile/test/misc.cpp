/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "context.h"
#include "hipfile-warnings.h"
#include "rocfile.h"
#include "rocfile-test.h"

#include <climits>
#include <gtest/gtest.h>
#include <memory>

using namespace rocFile;

// Put tests inside the macros to suppress the global constructor
// warnings
HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

struct RocFileVersioning : public RocFileUnopened {};

TEST_F(RocFileVersioning, Get)
{
    unsigned major = UINT_MAX;
    unsigned minor = UINT_MAX;
    unsigned patch = UINT_MAX;

    // Check for correct values
    ASSERT_EQ(rocFileGetVersion(&major, &minor, &patch), ROCFILE_SUCCESS);
    ASSERT_EQ(major, ROCFILE_VERSION_MAJOR);
    ASSERT_EQ(minor, ROCFILE_VERSION_MINOR);
    ASSERT_EQ(patch, ROCFILE_VERSION_PATCH);

    // NULL pointers should NOT produce errors
    ASSERT_EQ(rocFileGetVersion(nullptr, nullptr, nullptr), ROCFILE_SUCCESS);
}

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
