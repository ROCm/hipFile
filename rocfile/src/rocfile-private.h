/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "hipfile-types.h"
#include "rocfile.h"

#include <sys/types.h>

namespace rocFile {
enum class IoType;
}

// NOTE: This is an internal API that we don't document, even though it's
// "public" in the sense of being visible as a symbol in the deployed library

// Ensures that the driver is initialized without incrementing the reference
// count if the driver is already initialized. Used by hipFile to ensure its
// behaviour is consistent on AMD and NVIDIA
ROCFILE_API
void rocFileEnsureDriverInitPrivate();

ssize_t rocFileIo(rocFile::IoType type, rocFileHandle_t fh, const void *buffer_base, size_t size,
                  hoff_t file_offset, hoff_t buffer_offset);
