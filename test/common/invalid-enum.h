/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

// Create an invalid enum value without causing compiler errors. This works for
// enums that use int as the underlying type

#ifdef __clang__
__attribute__((noinline, no_sanitize("implicit-conversion")))
#endif
static int forceInt(int v) { return v; }

template <typename Enum>
Enum
invalidEnum(int value)
{
    return static_cast<Enum>(value);
}
