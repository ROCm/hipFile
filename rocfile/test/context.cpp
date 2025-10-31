/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "context.h"
#include "hip.h"
#include "hipfile-warnings.h"
#include <gtest/gtest.h>

using namespace rocFile;

// Put tests inside the macros to suppress the global constructor
// warnings
HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

TEST(RocFileContext, get_default_context)
{
    Hip *hip = Context<Hip>::get();
    ASSERT_NE(hip, nullptr);
}

TEST(RocFileContext, override_works_and_is_reverted_on_destruction)
{
    Hip *orig_context = Context<Hip>::get();
    {
        Hip                  hip;
        ContextOverride<Hip> ho(&hip);
        Hip                 *hip_context = Context<Hip>::get();
        ASSERT_EQ(&hip, hip_context);
    }
    Hip *hip_context = Context<Hip>::get();
    ASSERT_EQ(orig_context, hip_context);
}

TEST(RocFileContext, default_is_set_when_override_called_before_get)
{
    Hip *override_context;
    {
        Hip                  hip;
        ContextOverride<Hip> ho(&hip);
        override_context = Context<Hip>::get();
    }
    Hip *context = Context<Hip>::get();
    ASSERT_NE(override_context, context);
    ASSERT_NE(context, nullptr);
}

TEST(RocFileContext, context_throws_on_second_override)
{
    Hip                  hip;
    ContextOverride<Hip> ho(&hip);
    Hip                  hip2;
    ASSERT_THROW(ContextOverride<Hip> ho2(&hip2), std::runtime_error);
}

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
