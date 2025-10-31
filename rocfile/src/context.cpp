/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "context.h"
#include "hip.h"
#include "hipfile-warnings.h"
#include "state.h"
#include "sys.h"

namespace rocFile::context {

RocFileInit::RocFileInit()
{
    Context<Hip>::get();
    Context<Sys>::get();
    Context<DriverState>::get();
}

HIPFILE_WARN_NO_GLOBAL_CTOR_OFF
HIPFILE_WARN_NO_EXIT_DTOR_OFF
static RocFileInit *rocfile_init = Context<RocFileInit>::get();
HIPFILE_WARN_NO_EXIT_DTOR_ON
HIPFILE_WARN_NO_GLOBAL_CTOR_ON

}
