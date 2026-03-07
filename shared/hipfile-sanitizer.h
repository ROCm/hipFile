/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

/* Clang */
#if defined(__has_feature)
#  if __has_feature(undefined_behavior_sanitizer)
#    define BUILD_WITH_UBSAN 1
#  endif
#endif

/* GNU */
#if defined(__SANITIZE_UNDEFINED__)
#  define BUILD_WITH_UBSAN 1
#endif
