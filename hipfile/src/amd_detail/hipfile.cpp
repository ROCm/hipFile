/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "hipfile.h"
#include "hipfile-private.h"

#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <hip/hip_runtime_api.h>
#include <sys/types.h>
#include <vector>

using namespace std;

hipFileError_t
hipFileHandleRegister(hipFileHandle_t *fh, hipFileDescr_t *descr)
try {
    return rocFileHandleRegister(fh, descr);
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

void
hipFileHandleDeregister(hipFileHandle_t fh)
{
    (void)rocFileHandleDeregister(fh);
}

hipFileError_t
hipFileBufRegister(const void *buffer_base, size_t length, int flags)
try {
    return rocFileBufRegister(buffer_base, length, flags);
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileBufDeregister(const void *buffer_base)
try {
    auto error = rocFileBufDeregister(buffer_base);
    if (error.err == hipFileDriverNotInitialized) {
        error.err = hipFileDriverClosing;
    }
    return error;
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

ssize_t
hipFileRead(hipFileHandle_t fh, void *buffer_base, size_t size, hoff_t file_offset, hoff_t buffer_offset)
{
    auto result = rocFileRead(fh, buffer_base, size, file_offset, buffer_offset);
    if (result == -hipFileDriverNotInitialized) {
        // Match cuFile behaviour
        errno  = EINVAL;
        result = -1;
    }
    return result;
}

ssize_t
hipFileWrite(hipFileHandle_t fh, const void *buffer_base, size_t size, hoff_t file_offset,
             hoff_t buffer_offset)
{
    auto result = rocFileWrite(fh, buffer_base, size, file_offset, buffer_offset);
    if (result == -hipFileDriverNotInitialized) {
        // Match cuFile behaviour
        errno  = EINVAL;
        result = -1;
    }
    return result;
}
