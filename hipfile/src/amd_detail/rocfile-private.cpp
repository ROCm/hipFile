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
    hipFile::Context<hipFile::DriverState>::get()->ensureInitialized();
}
