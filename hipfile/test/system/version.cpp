/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "hipfile.h"
#include "hipfile-warnings.h"
#include "test-common.h"

#include <climits>
#include <gtest/gtest.h>
#include <memory>

HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

TEST(HipFileVersioning, Get)
{
    unsigned major = UINT_MAX;
    unsigned minor = UINT_MAX;
    unsigned patch = UINT_MAX;

    // Check for correct values
    ASSERT_EQ(hipFileGetVersion(&major, &minor, &patch), HIPFILE_SUCCESS);
    ASSERT_EQ(major, HIPFILE_VERSION_MAJOR);
    ASSERT_EQ(minor, HIPFILE_VERSION_MINOR);
    ASSERT_EQ(patch, HIPFILE_VERSION_PATCH);

    // NULL pointers should NOT produce errors
    ASSERT_EQ(hipFileGetVersion(nullptr, nullptr, nullptr), HIPFILE_SUCCESS);

    // hipFileGetBackendVersion() succeeds and returns a value >= 0
    //
    // We can't reliably predict what the version number will be for
    // an arbitrary library, but it probably won't be negative and
    // checking for >= 0 ensures the -1 initialization value is
    // overwritten.
    int backend_version = -1;
    ASSERT_EQ(hipFileGetBackendVersion(&backend_version), HIPFILE_SUCCESS);
    ASSERT_GE(backend_version, 0);

    // NULL pointer returns correct error
    ASSERT_EQ(hipFileGetBackendVersion(nullptr), HipFileOpError(hipFileInvalidValue));
}

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
