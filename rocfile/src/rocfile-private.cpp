/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "context.h"
#include "rocfile-private.h"
#include "state.h"

void
rocFileEnsureDriverInitPrivate()
{
    rocFile::context::Context<rocFile::DriverState>::get()->ensureInitialized();
}
