/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <sys/types.h>

// ***********************************************************************
//  PLATFORM-INDEPENDENT TYPES
// ***********************************************************************

/*!
 * @brief Platform-independent offset type
 * @ingroup core
 */
#ifndef _WIN32
typedef off_t hoff_t;
#else
typedef __int64 hoff_t;
#endif
