/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "hipfile.h"

// The major version number should ideally remain unchanged. Increment the
// major version only for fundamental changes to the hipFileDispatchTable
// struct, such as altering the type or name of an existing member variable.
#define HIPFILE_RUNTIME_API_TABLE_MAJOR_VERSION 0

// Increment the step versoin when new runtime API functions are added.
// If the corresponding table versoin increases, reset the step version
// to zero.
#define HIPFILE_RUNTIME_API_TABLE_STEP_VERSION 0

// hipFile API interface
// *** API CALLS GO HERE ***

// hipFile API dispatch table
struct hipFileDispatchTable {
    // HIPFILE_RUNTIME_API_TABLE_STEP_VERSION == 0
    size_t size;
    // *** API CALLS GO HERE ***

    // PLEASE DO NOT EDIT ABOVE!

    // HIPFILE_RUNTIME_API_TABLE_STEP_VERSION == 1

    // *******************************************************************************************
    //                                            READ BELOW
    // *******************************************************************************************
    // Please keep this text at the end of the structure:

    // 1. Do not reorder any existing members
    // 2. Increase the step version definition before adding new members
    // 3. Insert new members under the appropriate step version comment
    // 4. Generate a comment for the next step version
    // 5. Add a "PLEASE DO NOT EDIT ABOVE!" comment
    // *******************************************************************************************
};
