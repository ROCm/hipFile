/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "hipfile.h"

#include <memory>
#include <sys/types.h>
#include <vector>

namespace hipFile {
enum class IoType;
struct Backend;
}

// NOTE: This is an internal API that we don't document, even though it's
// "public" in the sense of being visible as a symbol in the deployed library

// Ensures that the driver is initialized without incrementing the reference
// count if the driver is already initialized. Used by hipFile to ensure its
// behaviour is consistent on AMD and NVIDIA
HIPFILE_API
void hipFileEnsureDriverInitPrivate();

ssize_t rocFileIo(hipFile::IoType type, rocFileHandle_t fh, const void *buffer_base, size_t size,
                  hoff_t file_offset, hoff_t buffer_offset,
                  const std::vector<std::shared_ptr<hipFile::Backend>> &backends);
