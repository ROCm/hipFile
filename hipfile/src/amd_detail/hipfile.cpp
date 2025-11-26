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

hipFileError_t
hipFileBatchIOSetUp(hipFileBatchHandle_t *batch_idp, unsigned max_nr)
try {
    return rocFileBatchIOSetUp(batch_idp, max_nr);
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileBatchIOSubmit(hipFileBatchHandle_t batch_idp, unsigned nr, hipFileIOParams_t *iocbp, unsigned flags)
try {
    auto           rf_batch_idp = batch_idp;
    hipFileError_t status;

    if (iocbp) {
        vector<hipFileIOParams_t> io_params(nr);

        for (unsigned i = 0; i < nr; i++) {
            io_params[i] = iocbp[i];
        }

        status = rocFileBatchIOSubmit(rf_batch_idp, nr, io_params.data(), flags);
    }
    else {
        status = rocFileBatchIOSubmit(rf_batch_idp, nr, nullptr, flags);
    }

    return status;
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileBatchIOGetStatus(hipFileBatchHandle_t batch_idp, unsigned min_nr, unsigned *nr,
                        hipFileIOEvents_t *iocbp, struct timespec *timeout)
try {
    auto           rf_batch_idp = batch_idp;
    hipFileError_t status;

    if (iocbp) {
        vector<hipFileIOEvents_t> io_events(*nr);

        status = rocFileBatchIOGetStatus(rf_batch_idp, min_nr, nr, io_events.data(), timeout);

        if (status.err == hipFileSuccess) {
            for (unsigned i = 0; i < *nr; i++) {
                iocbp[i] = io_events[i];
            }
        }
    }
    else {
        status = rocFileBatchIOGetStatus(rf_batch_idp, min_nr, nr, nullptr, timeout);
    }

    return status;
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

hipFileError_t
hipFileBatchIOCancel(hipFileBatchHandle_t batch_idp)
try {
    return rocFileBatchIOCancel(batch_idp);
}
catch (...) {
    return {hipFileInternalError, hipSuccess};
}

void
hipFileBatchIODestroy(hipFileBatchHandle_t batch_idp)
{
    (void)rocFileBatchIODestroy(batch_idp);
}
