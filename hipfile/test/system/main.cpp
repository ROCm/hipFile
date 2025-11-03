/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "hipfile-warnings.h"
#include "test-options.h"

#include <gtest/gtest.h>

extern SystemTestOptions test_env;
HIPFILE_WARN_NO_GLOBAL_CTOR_OFF
HIPFILE_WARN_NO_EXIT_DTOR_OFF
SystemTestOptions test_env;
HIPFILE_WARN_NO_EXIT_DTOR_ON
HIPFILE_WARN_NO_GLOBAL_CTOR_ON

int
main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);

    test_env.parseTestOptions(argc, argv);

    return RUN_ALL_TESTS();
}
